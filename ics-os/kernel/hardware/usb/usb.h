/*
  Name: usb.h
  Description: USB UHCI host and Mass Storage Class (Bulk-Only) support.
               Used to boot ICS-OS from a USB thumb drive and mount that
               drive as the root filesystem.
*/

#ifndef ICSOS_USB_H
#define ICSOS_USB_H

int usb_init(void);
int usb_storage_available(void);

#endif
