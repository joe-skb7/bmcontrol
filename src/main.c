#include <std/quirks.h>
#include <std/int.h>
#include <std/bool.h>
#include <os/time.h>
#include <os/usb.h>

#include <stdio.h>
#include <string.h>

#define VERSION				"1.1.1"
#define VENDOR_ID			0x16c0
#define PRODUCT_ID			0x05df

#define INTFACE				0
#define ONEWIRE_REPEAT			5
#define USB_REPEAT			5
/* How long to wait for USB message to complete before timeout, msec */
#define MSG_TIMEOUT			5000
/* Workaround sleep for libusb errata, msec */
#define QUIRK_LIBUSB_SLEEP		20

/*
 * Request types
 */
#define USB_RT_IN  (USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_ENDPOINT_IN)
#define USB_RT_OUT (USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_ENDPOINT_OUT)

/*
 * HID class requests
 */
#define HID_REQ_GET_REPORT		0x01
#define HID_REQ_SET_REPORT		0x09

#define FEATURE_REPORT_TYPE		(0x03 << 8)

static unsigned char USB_BUFI[8];
static unsigned char USB_BUFO[8];
static uint64_t ONEWIRE_ROM[40];
static int ONEWIRE_COUNT;
static float T;

usb_dev_handle *find_lvr_winusb();

/*
 * In "Device class definition for HID" specification,
 * "7.2.1 Get_Report Request" chapter: see Remarks.
 */
static inline uint16_t feature_report(uint8_t report_id)
{
	/* for example: 0x300: 0x3 = feature, 00 = report ID */
	return FEATURE_REPORT_TYPE | report_id;
}

static usb_dev_handle *setup_libusb_access(void)
{
     usb_dev_handle *lvr_winusb;
     usb_set_debug(0);
     usb_init();
     usb_find_busses();
     usb_find_devices();

     if(!(lvr_winusb = find_lvr_winusb())) {
                return NULL;
        }

#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
	usb_detach_kernel_driver_np(lvr_winusb,0);
#endif

        if (usb_set_configuration(lvr_winusb, 1) < 0) {
                fprintf(stderr, "Could not set configuration 1 : \n");
                return NULL;
        }

        if (usb_claim_interface(lvr_winusb, INTFACE) < 0) {
                fprintf(stderr, "Could not claim interface: \n");
                return NULL;
        }
        return lvr_winusb;
 }

usb_dev_handle *find_lvr_winusb(void)
{
      struct usb_bus *bus;
         struct usb_device *dev;
         for (bus = usb_busses; bus; bus = bus->next) {
         for (dev = bus->devices; dev; dev = dev->next) {
                        if (dev->descriptor.idVendor == VENDOR_ID &&
                                dev->descriptor.idProduct == PRODUCT_ID ) {
                                usb_dev_handle *handle;
                                if (!(handle = usb_open(dev))) {
                                        fprintf(stderr, "Could not open USB device\n");
                                        return NULL;
                                }
                                return handle;
                        }
                }
        }
        return NULL;
}

static usb_dev_handle *lvr_winusb = NULL;

static void USB_PAUSE(unsigned int msecs)
{
    struct timespec req;

    req.tv_sec = msecs / 1000;
    req.tv_nsec = (msecs % 1000) * 1000000;

    if (nanosleep(&req, NULL) == -1) {
        perror("Error occurred while sleeping");
        exit(EXIT_FAILURE);
    }
}

/* очистка буферов приёма и передачи */
static void USB_BUF_CLEAR(void)
{
	memset(USB_BUFI, 0, sizeof(USB_BUFI));
	memset(USB_BUFO, 0, sizeof(USB_BUFO));
}

/* чтение в буфер из устройства */
static int USB_GET_FEATURE(void)
{
    int RESULT = 0;
    int i=USB_REPEAT;   /*  число попыток */
    while (!RESULT && i--) {
        RESULT = usb_control_msg(lvr_winusb, USB_RT_IN, HID_REQ_GET_REPORT,
                feature_report(0), 0, (char *)USB_BUFI, sizeof(USB_BUFI),
                MSG_TIMEOUT);
        USB_PAUSE(QUIRK_LIBUSB_SLEEP);
    }
    if (!RESULT) fprintf(stderr, "Error reading from device\n");
/*
    printf("read ");
    for(int i=0;i<8;i++) printf("%x ",USB_BUFI[i]);
    printf("\n");
*/
    return RESULT;
}

/* запись из буфера в устройство */
static int USB_SET_FEATURE(void)
{
    int RESULT=0;
    RESULT = usb_control_msg(lvr_winusb, USB_RT_OUT, HID_REQ_SET_REPORT,
            feature_report(0), 0, (char *)USB_BUFO, sizeof(USB_BUFO),
            MSG_TIMEOUT);
    USB_PAUSE(QUIRK_LIBUSB_SLEEP);
    if (!RESULT) fprintf(stderr, "Error writing to device\n");
/*
    printf("write ");
    for(int i=0;i<8;i++) printf("%x ",USB_BUFO[i]);
    printf("\n");
*/
    return RESULT;
}

/* чтение состояния порта, 2ms */
static bool USB_GET_PORT(unsigned char *PS)
{
    bool RESULT=false;
    int i=USB_REPEAT;   /* число попыток */

    USB_BUF_CLEAR();
    USB_BUFO[0]=0x7E;
    while (!RESULT && i--)
        if (USB_SET_FEATURE())
            if (USB_GET_FEATURE()) {
                if (USB_BUFI[0]==0x7E) { *PS=USB_BUFI[1]; RESULT=USB_BUFI[2]==*PS; }
                else RESULT=false;
            }
    if (!RESULT) fprintf(stderr, "Error reading PORT\n");
    return RESULT;
}

/* запись состояния порта, 2ms */
static bool USB_SET_PORT(unsigned char PS)
{
    bool RESULT=false;
    int i=USB_REPEAT;   /* число попыток */

    USB_BUF_CLEAR();
    USB_BUFO[0]=0xE7;
    USB_BUFO[1]=PS;
    while (!RESULT && i--)
        if (USB_SET_FEATURE())
            if (USB_GET_FEATURE())
                 { RESULT=(USB_BUFI[0]==0xE7)&(USB_BUFI[1]==PS)&(USB_BUFI[2]==PS); }
    if (!RESULT) fprintf(stderr, "Error writing PORT\n");
    return RESULT;
}

/* чтение группового кода устройства, 2ms */
static bool USB_GET_FAMILY(unsigned char *FAMILY)
{
    bool RESULT=false;
    int i=USB_REPEAT;   /* число попыток */

    USB_BUF_CLEAR();
    USB_BUFO[0]=0x1D;
    while (!RESULT && i--)
        if (USB_SET_FEATURE())
            if (USB_GET_FEATURE()) {
                if (USB_BUFI[0]==0x1D) { RESULT=true; *FAMILY=USB_BUFI[1]; }
                else RESULT=false;
            }
    if (!RESULT) fprintf(stderr, "Error reading FAMILY\n");
    return RESULT;
}

/* чтение номера версии прошивки, 2ms */
static bool USB_GET_SOFTV(unsigned int *SV)
{
    bool RESULT=false;
    int i=USB_REPEAT;   /* число попыток */

    USB_BUF_CLEAR();
    USB_BUFO[0]=0x1D;
    while (!RESULT && i--)
        if (USB_SET_FEATURE())
            if (USB_GET_FEATURE()) {
                if (USB_BUFI[0]==0x1D) { RESULT=true; *SV=USB_BUFI[2]+(USB_BUFI[3]>>8); }
                else RESULT=false;
            }
    if (!RESULT) fprintf(stderr, "Error reading firmware version\n");
    return RESULT;
}

/* чтение ID устройства, 2ms */
static bool USB_GET_ID(unsigned int *ID)
{
    bool RESULT=false;
    int i=USB_REPEAT;   /* число попыток */

    USB_BUF_CLEAR();
    USB_BUFO[0]=0x1D;
    while (!RESULT && i--)
        if (USB_SET_FEATURE())
            if (USB_GET_FEATURE()) {
                if (USB_BUFI[0]==0x1D) { RESULT=true; *ID=(USB_BUFI[4]<<24)+(USB_BUFI[5]<<16)+(USB_BUFI[6]<<8)+USB_BUFI[7]; }
                else RESULT=false;
            }
    if (!RESULT) fprintf(stderr, "Error reading device ID\n");
    return RESULT;
}

/* чтение EEPROM */
static bool USB_EE_RD(unsigned char ADR, unsigned char *DATA)
{
    bool RESULT=false;
    int i=USB_REPEAT;   /* число попыток */

    USB_BUF_CLEAR();
    USB_BUFO[0]=0xE0;
    USB_BUFO[1]=ADR;
    while (!RESULT && i--)
        if (USB_SET_FEATURE())
            if (USB_GET_FEATURE()) { RESULT=(USB_BUFI[0]==0xE0)&(USB_BUFI[1]==ADR); *DATA=USB_BUFI[2]; }
    if (!RESULT) fprintf(stderr, "Error reading EEPROM\n");
    return RESULT;
}

/* запись EEPROM, 17ms */
static bool USB_EE_WR(unsigned char ADR,unsigned  char DATA)
{
    bool RESULT=false;
    int i=USB_REPEAT;   /* число попыток */

    USB_BUF_CLEAR();
    USB_BUFO[0]=0x0E;
    USB_BUFO[1]=ADR;    USB_BUFO[2]=DATA;
    while (!RESULT && i--)
        if (USB_SET_FEATURE())
            {
            USB_PAUSE(15); /* на запись в EEPROM */
            if (USB_GET_FEATURE()) RESULT=(USB_BUFI[0]==0x0E)&(USB_BUFI[1]==ADR)&(USB_BUFI[2]==DATA);
            } else RESULT=false;
    if (!RESULT) fprintf(stderr, "Error writing EEPROM\n");
    return RESULT;
}

/* RESET, ~3ms */
static bool OW_RESET(void)
{
    bool RESULT=false;
    unsigned char N=ONEWIRE_REPEAT;

    USB_BUF_CLEAR();
    USB_BUFO[0]=0x18;    USB_BUFO[1]=0x48;


    while (!RESULT && N--)
        if (USB_SET_FEATURE())
            {
            USB_PAUSE(1);
            if (USB_GET_FEATURE()) {
               RESULT=(USB_BUFI[0]==0x18)&(USB_BUFI[1]==0x48)&(USB_BUFI[2]==0x00);
            }
                else RESULT=false;
            }
    if (!RESULT) fprintf(stderr, "Error OW_RESET\n");
    return RESULT;
}

/* чтение 2-x бит, 3ms */
static bool OW_READ_2BIT(unsigned char *B)
{    bool RESULT=false;
    USB_BUF_CLEAR();
    USB_BUFO[0]=0x18;    USB_BUFO[1]=0x82;
    USB_BUFO[2]=0x01;    USB_BUFO[3]=0x01;
    if (USB_SET_FEATURE())
        {
        USB_PAUSE(1);
        if (USB_GET_FEATURE())
            { RESULT=(USB_BUFI[0]==0x18)&(USB_BUFI[1]==0x82); *B=(USB_BUFI[2]&0x01)+((USB_BUFI[3]<<1)&0x02); }
        }
    if (!RESULT) fprintf(stderr, "Error OW_READ_2BIT\n");
    return RESULT;
}

/* чтение байта, 3ms */
static bool OW_READ_BYTE(unsigned char *B)
{    bool RESULT=false;
    USB_BUF_CLEAR();
    USB_BUFO[0]=0x18;    USB_BUFO[1]=0x88;    USB_BUFO[2]=0xFF;
    if (USB_SET_FEATURE())
        {
        USB_PAUSE(1);
        if (USB_GET_FEATURE())
            { RESULT=(USB_BUFI[0]==0x18)&(USB_BUFI[1]==0x88); *B=USB_BUFI[2]; }
        }
    if (!RESULT) fprintf(stderr, "Error OW_READ_BYTE\n");
    return RESULT;
}

/* чтение 4 байта, 4ms */
static bool OW_READ_4BYTE(unsigned long *B)
{    bool RESULT=false;
    USB_BUF_CLEAR();
    USB_BUFO[0]=0x18;    USB_BUFO[1]=0x84;    USB_BUFO[2]=0xFF;
    USB_BUFO[3]=0xFF;    USB_BUFO[4]=0xFF;    USB_BUFO[5]=0xFF;
    if (USB_SET_FEATURE())
        {
        USB_PAUSE(2);
        if (USB_GET_FEATURE())
            { RESULT=(USB_BUFI[0]==0x18)&(USB_BUFI[1]==0x84); *B=USB_BUFI[2]+(USB_BUFI[3]<<8)+(USB_BUFI[4]<<16)+(USB_BUFI[5]<<24); }
        }
    if (!RESULT) fprintf(stderr, "Error OW_READ_4BYTE\n");
    return RESULT;
}

/* запись бита, 3ms */
static bool OW_WRITE_BIT(unsigned char B)
{    bool RESULT=false;
    USB_BUF_CLEAR();
    USB_BUFO[0]=0x18;    USB_BUFO[1]=0x81;    USB_BUFO[2]=B&0x01;
    if (USB_SET_FEATURE())
        {
        USB_PAUSE(1);
        if (USB_GET_FEATURE())
            RESULT=(USB_BUFI[0]==0x18)&(USB_BUFI[1]==0x81)&((USB_BUFI[2]&0x01)==(B&0x01));
        }
    if (!RESULT) fprintf(stderr, "Error OW_WRITE_BIT\n");
    return RESULT;
}

/* запись байта, 3ms */
static bool OW_WRITE_BYTE(unsigned char B)
{    bool RESULT=false;
    USB_BUF_CLEAR();
    USB_BUFO[0]=0x18;    USB_BUFO[1]=0x88;    USB_BUFO[2]=B;
    if (USB_SET_FEATURE())
        {
        USB_PAUSE(1);
        if (USB_GET_FEATURE())
            RESULT=(USB_BUFI[0]==0x18)&(USB_BUFI[1]==0x88)&(USB_BUFI[2]==B);
        }
    if (!RESULT) fprintf(stderr, "Error OW_WRITE_BYTE\n");
    return RESULT;
}

/* запись 4 байта, 4ms */
static bool OW_WRITE_4BYTE(unsigned long B)
{    bool RESULT=false;
    unsigned char D0, D1, D2, D3;
    D0=B&0xFF;
    D1=(B>>8) &0xFF;
    D2=(B>>16)&0xFF;
    D3=(B>>24)&0xFF;
    USB_BUF_CLEAR();
    USB_BUFO[0]=0x18;    USB_BUFO[1]=0x84;    USB_BUFO[2]=D0;
    USB_BUFO[3]=D1;
    USB_BUFO[4]=D2;
    USB_BUFO[5]=D3;
    if (USB_SET_FEATURE())
        {
        USB_PAUSE(2);
        if (USB_GET_FEATURE())
            RESULT=(USB_BUFI[0]==0x18)&(USB_BUFI[1]==0x84)&(USB_BUFI[2]==D0)&(USB_BUFI[3]==D1)&(USB_BUFI[4]==D2)&(USB_BUFI[5]==D3);
        }
    if (!RESULT) fprintf(stderr, "Error OW_WRITE_4BYTE\n");
    return RESULT;
}

/* подчсёт CRC для DALLAS */
static unsigned char CRC8(unsigned char CRC, unsigned char D)
{    unsigned char R=CRC;
     int i;

    for (i=0; i<8; i++)
        if (((R^(D>>i))&0x01)==0x01) R=((R^0x18)>>1)|0x80;
            else R=(R>>1)&0x7F;
    return R;
}

/* выбор прибора по ROM, 14ms */
static bool MATCH_ROM(uint64_t ROM)
{    bool RESULT=false;
    uint64_t T=ROM;
    unsigned char N=ONEWIRE_REPEAT;
    while (!RESULT && N--)
        if (OW_RESET())
            if (OW_WRITE_BYTE(0x55))
                if (OW_WRITE_4BYTE(T&0xFFFFFFFF))
                    RESULT=OW_WRITE_4BYTE((T>>32)&0xFFFFFFFF);
    if (!RESULT) fprintf(stderr, "Error MATCH_ROM\n");
    return RESULT;
}

/* поиск ROM, 1 dev - 410ms, 5 dev - 2.26s, 20 dev - 8.89s */
static bool SEARCH_ROM(uint64_t ROM_NEXT, int PL)
{    bool RESULT=false;
    unsigned char N=ONEWIRE_REPEAT;
    unsigned char BIT;
    bool CL[64] = { false };
    int i;
    uint64_t RL[64];
    uint64_t B1=1, CRC, ROM;

    while (!RESULT && N--)
        {
        ROM=0;
        if (OW_RESET()) RESULT=OW_WRITE_BYTE(0xF0);
        if (RESULT)
            for (i=0; i<64; i++)
                if (RESULT) {
                    if (OW_READ_2BIT(&BIT))
                        switch (BIT&0x03)
                            {
                            case 0 :
                                {   /* коллизия есть */
                                if (PL<i) {CL[i]=true; RL[i]=ROM;}
                                if (PL>=i) BIT=(ROM_NEXT>>i)&0x01; else BIT=0;
                                if(!OW_WRITE_BIT(BIT)) { RESULT=false; i=64; }
                                if (BIT==1) ROM=ROM+(B1<<i);
                                break;
                                }
                            case 1 : { if (!OW_WRITE_BIT(0x01)) { RESULT=false; i=64; } else ROM=ROM+(B1<<i); break;}
                            case 2 : { if (!OW_WRITE_BIT(0x00)) { RESULT=false; i=64; } break;}
                            case 3 : { RESULT=false; i=64; break;}   /* нет на линии */
                            }
                    else { RESULT=false; i=64; }
                }
        if (ROM==0) RESULT=false;
        if (RESULT) { int j; CRC=0; for (j=0; j<8; j++) CRC=CRC8(CRC, (ROM>>(j*8))&0xFF); RESULT=CRC==0; }
        }
    if (!RESULT) fprintf(stderr, "Error SEARCH_ROM\n");
        else ONEWIRE_ROM[ONEWIRE_COUNT++]=ROM;
    /* рекурентный вызов поиска */
    for (i=0; i<64; i++)
        if (CL[i]) SEARCH_ROM(RL[i]|(B1<<i), i);
    return RESULT;
}

/* пропуск ROM-команд, старт измерения температуры, 9ms */
static bool SKIP_ROM_CONVERT(void)
{    bool RESULT=false;
    unsigned char N=ONEWIRE_REPEAT;
    while (!RESULT && N--)
        if (OW_RESET())
            if (OW_WRITE_BYTE(0xCC))
                RESULT=OW_WRITE_BYTE(0x44);
    if (!RESULT) fprintf(stderr, "Error SKIP_ROM_CONVERT\n");
    return RESULT;
}

/* чтение температуры, 28ms */
static bool GET_TEMPERATURE(uint64_t ROM, float *T)
{    uint64_t CRC;
    unsigned long L1, L2;
    unsigned char L3;
    unsigned char FAMILY=ROM&0xFF;
    bool RESULT=false;
    unsigned char N=ONEWIRE_REPEAT;
    while (!RESULT && N--)
        if (MATCH_ROM(ROM))
            if (OW_WRITE_BYTE(0xBE))
                    if (OW_READ_4BYTE(&L1))
                        if (OW_READ_4BYTE(&L2))
                            if (OW_READ_BYTE(&L3))
                                {
                                int i;
                                short K;

                                CRC=0;
                                for (i=0; i<4; i++) CRC=CRC8(CRC, (L1>>(i*8))&0xFF);
                                for (i=0; i<4; i++) CRC=CRC8(CRC, (L2>>(i*8))&0xFF);
                                CRC=CRC8(CRC, L3);
                                RESULT=CRC==0;

                                K=L1&0xFFFF;
                                /* DS18B20 +10.125=00A2h, -10.125=FF5Eh
                                 * DS18S20 +25.0=0032h, -25.0=FFCEh
                                 * K=0x0032;
                                */
                                *T=1000;     /* для неопознанной FAMILY датчик отсутствует */
                                if ((FAMILY==0x28)|(FAMILY==0x22)) *T=K*0.0625;  /* DS18B20 | DS1822 */
                                if (FAMILY==0x10) *T=K*0.5;                      /* DS18S20 | DS1820 */
                                }
    if (!RESULT) fprintf(stderr, "Error GET_TEMPERATURE\n");
    return RESULT;
}


static int read_ports(void)
 {
    unsigned char PS;
    if(USB_GET_PORT(&PS)) {
        if((PS==8)|(PS==24)) printf("Port1 is on\n");
        else printf("Port1 is off\n");
        if(PS>=16) printf("Port2 is on\n");
        else printf("Port2 is off\n");
        return 1;
    }
    return 0;
 }

static int set_port(int num, bool stat)
 {
    unsigned char PS;
    bool ret = 0;

    if (!USB_GET_PORT(&PS))
        { fprintf(stderr, "Error USB_GET_PORT\n"); return 0; }
    /* включение / выключение */
    if ((num==1)&(stat==1))  { PS=PS|0x08; ret = USB_SET_PORT(PS); }
    else if ((num==1)&(stat==0)) { PS=PS&0x10; ret = USB_SET_PORT(PS); }
    else if ((num==2)&(stat==1))  { PS=PS|0x10; ret = USB_SET_PORT(PS); }
    else if ((num==2)&(stat==0)) { PS=PS&0x08; ret = USB_SET_PORT(PS); }
    if(!ret) return 0;
    printf("Status port changed\n");
    return 1;
}

static int device_info(void)
{
	int ret=0;
	unsigned int SV,ID;
	unsigned char FAMILY;
	ret = USB_GET_SOFTV(&SV);
    if(ret) printf("Firmware: %xh\n", SV);
    ret = USB_GET_FAMILY(&FAMILY);
    if(ret) printf("USB series: %xh\n", FAMILY);
    ret = USB_GET_ID(&ID);
    if(ret) printf("USB identifier: %xh\n", ID);
    return 1;
}

static int scan(void) {
    int i;

    SEARCH_ROM(0, 0);

    for(i=1;i<=ONEWIRE_COUNT;i++) {
            printf("temp_id%d = %x%x\n", i,
                (unsigned)((ONEWIRE_ROM[i-1] >> 32) & 0xFFFFFFFF),
                (unsigned)(ONEWIRE_ROM[i-1] & 0xFFFFFFFF));
    }

    return 1;
}

static int temp(uint64_t ROM) {
    int ret;
    ONEWIRE_COUNT = 1;
    ONEWIRE_ROM[0] = ROM;
    SKIP_ROM_CONVERT();
    ret = GET_TEMPERATURE(ONEWIRE_ROM[0], &T);
    if(ret) printf("%f\n",T);
    return ret;
}

static int ports_save(void)
{
    unsigned char PS;
    if (USB_GET_PORT(&PS)) {
        if (USB_EE_WR(0x04, PS)) {printf("Status ports saved\n");return 1;}
    }
    fprintf(stderr, "Error saving ports status\n");
    return 0;
}

static int delay_get(void) {
    unsigned char B = 0;

    USB_EE_RD(0x05, &B);
    printf("%d\n", B);
    return 1;
}

static int delay_set(int B) {
    if(((B<5)|(B>255))&(B!=0)) {
        fprintf(stderr, "Wrong num %d\n",B);
        return 0;
    }
    if (USB_EE_WR(0x05, B)) {printf("Delay changed\n");return 1;}
    return 0;
}


static uint64_t HexStringToUInt(char* s)
{
uint64_t v = 0;
char c;

while ((c = *s++))
{
if (c < '0') return 0; /* invalid character */
if (c > '9') /* shift alphabetic characters down */
{
if (c >= 'a') c -= 'a' - 'A'; /* upper-case 'a' or higher */
if (c > 'Z') return 0; /* invalid character */
if (c > '9') c -= 'A'-1-'9'; /* make consecutive with digits */
if (c < '9' + 1) return 0; /* invalid character */
}
c -= '0'; /* convert char to hex digit value */
v = v << 4; /* shift left by a hex digit */
v += c; /* add the current digit */
}

return v;
}

int main( int argc, char **argv)
{
    lvr_winusb = setup_libusb_access();

        if(argc==1) {
            printf("Temperature sensor BM1707 control v" VERSION "\n");
            if(lvr_winusb!=NULL) {
                printf("Device has been plugged\n");
            }
            else printf("Device unplugged\n");
            printf("\nUsage: bmcontrol [options]\n\n");
            printf("   info                          Show device information\n");
            printf("   scan                          Scaning temperature sensors\n");
            printf("   temp <id>                     Show temperature sensor <id> \n");
            printf("   ports                         Show ports status\n");
            printf("   pset <port> <status>          Set off/on port status.\n");
            printf("                                 Correct value: port [1, 2] status [0, 1]. \n");
            printf("   psave                         Save ports status in EEPROM\n");
            printf("   delay                         Get delay time of device before power save\n");
            printf("   delay <5-255>                 Set delay time of device before power save\n");
        }
        else if(lvr_winusb!=NULL){
             if(strcmp(argv[1],"ports") == 0) read_ports();
             else if(strcmp(argv[1],"info") == 0) device_info();
             else if(strcmp(argv[1],"scan") == 0) scan();
             else if(strcmp(argv[1],"psave") == 0) ports_save();
             else if((strcmp(argv[1],"temp") == 0)&&(argv[2])) {
                  uint64_t rom=0;
                  rom = HexStringToUInt(argv[2]);
                  temp(rom);
             }
             else if((strcmp(argv[1],"pset") == 0)&(argc==4) && (argv[2] != NULL)) set_port(atoi((const char*) argv[2]), (bool) atoi((const char*) argv[3]));
             else if((strcmp(argv[1],"delay") == 0)&(argc==2)) delay_get();
             else if((strcmp(argv[1],"delay") == 0)&(argc==3) && (argv[2] != NULL)) delay_set( atoi(argv[2]));
             else fprintf(stderr, "Wrong command %s.\n", argv[1]);
        }
        else {
             fprintf(stderr, "Device not plugged\n");
             exit(-1);
        }
        if(lvr_winusb!=NULL) {
            usb_release_interface(lvr_winusb, 0);
            usb_close(lvr_winusb);
        }
        return 0;
}

