#ifndef BSBB_USB_H
#define BSBB_USB_H

void usb_platform_setup(void);
void usb_set_connected(bool connected);
void usb_device_isr(void);
void usb_host_isr(void);

void usb_setup(void);
void usb_loop(void);

#endif // BSBB_USBH_H
