#ifndef OS_USB_H
#define OS_USB_H

#if defined(__linux__)
# include <usb.h>
#elif defined(_WIN32)
# include <lusb0_usb.h>
#else
/* Try to use the same as for Linux */
# include <usb.h>
#endif

#endif /* OS_USB_H*/
