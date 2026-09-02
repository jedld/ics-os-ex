/*
  Name: usb.h
  Description: USB UHCI/xHCI host and Mass Storage Class (Bulk-Only) support.
               Used to boot ICS-OS from a USB thumb drive and mount that
               drive as the root filesystem.
*/

#ifndef ICSOS_USB_H
#define ICSOS_USB_H

int usb_init(void);
int usb_storage_available(void);
int usb_start_hotplug_monitor(void);
int usb_xhci_mounted_disconnect_selftest(void);

#endif
