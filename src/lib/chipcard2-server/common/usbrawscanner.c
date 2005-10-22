/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id$
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "usbrawscanner_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/directory.h>
#include <gwenhywfar/buffer.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#ifdef USE_LIBUSB
# include <usb.h>
#endif


static int lc_usbrawscanner__initcount=0;


LC_DEVSCANNER *LC_UsbRawScanner_new() {
  LC_DEVSCANNER *sc;

#ifdef USE_LIBUSB
  if (lc_usbrawscanner__initcount++==0)
    usb_init();
#endif

  sc=LC_DevScanner_new();
  LC_DevScanner_SetReadDevsFn(sc, LC_UsbRawScanner_ReadDevs);

  return sc;
}



int LC_UsbRawScanner_ReadDevs(LC_DEVSCANNER *sc, LC_DEVICE_LIST *dl) {
#ifdef USE_LIBUSB
  struct usb_bus *bus;
  struct usb_device *dev;
  int count=0;

  usb_find_busses();
  usb_find_devices();

  for (bus = usb_busses; bus; bus = bus->next) {
    for (dev=bus->devices; dev; dev = dev->next) {
      LC_DEVICE *d;

      DBG_VERBOUS(0, "Got device %04x/%04x/%04x/%04x",
                  0, dev->descriptor.bcdDevice,
                  dev->descriptor.idVendor,
                  dev->descriptor.idProduct);
      d=LC_Device_new(LC_Device_BusType_UsbRaw,
                      bus->location,              /* bus id */
                      dev->descriptor.bcdDevice,  /* device id */
                      dev->descriptor.idVendor,
                      dev->descriptor.idProduct);
      LC_Device_SetDevicePos(d, count++);
      LC_Device_List_Add(d, dl);
    }
  }
#else
  DBG_VERBOUS(0, "UsbRaw scanner not supported (no LibUSB)");
#endif
  return 0;
}




