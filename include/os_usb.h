#ifndef OS_USB_H
#define OS_USB_H

#if defined(__linux__)
# include <usb.h>
#elif defined(_WIN32)
# include <lusb0_usb.h>
#else
# pragma message("Currently only Linux and Windows build is supported. " \
                 "Trying to use the same \"usb\" header as on Linux.")
# include <usb.h>
#endif

#endif /* OS_USB_H*/
