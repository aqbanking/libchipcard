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


#include "usbmonitor_p.h"
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



LC_DEVSCANNER *LC_UsbRawScanner_new() {
  LC_DEVSCANNER *sc;

  sc=LC_DevScanner_new();
  LC_DevScanner_SetReadDevsFn(sc, LC_PciScanner_ReadDevs);

  return sc;
}



int LC_USBMonitor_Read_UsbDevices(LC_USBDEVICE_LIST *dl) {
#ifdef USE_LIBUSB
  struct usb_bus *bus;
  struct usb_device *dev;
  int changes;
  int count;

  usb_find_busses();
  changes=usb_find_devices();
  if (!changes)
    return 1;

  count=0;
  for (bus = usb_busses; bus; bus = bus->next) {
    for (dev=bus->devices; dev; dev = dev->next) {
      LC_USBDEVICE *d;

      DBG_DEBUG(0, "Got device %04x/%04x/%04x/%04x",
                0, dev->descriptor.bcdDevice,
                dev->descriptor.idVendor,
                dev->descriptor.idProduct);
      d=LC_USBDevice_new(0, dev->descriptor.bcdDevice,
                         dev->descriptor.idVendor,
                         dev->descriptor.idProduct);
      d->devicePos=count++;
      LC_USBDevice_List_Add(d, dl);
    }
  }



int LC_USBMonitor_Scan(LC_USBMONITOR *um) {
  LC_USBDEVICE_LIST *dl;
  LC_USBDEVICE *d;
  int rv;

  LC_USBDevice_List_Clear(um->newDevices);
  LC_USBDevice_List_Clear(um->lostDevices);

  dl=LC_USBDevice_List_new();

  rv=LC_USBMonitor_Read_UsbDevices(dl);
  if (rv==-1) {
    DBG_INFO(0, "here");
    LC_USBDevice_List_free(dl);
    return -1;
  }
  else if (rv==1) {
    LC_USBDevice_List_free(dl);
    return 1;
  }

  /* find new devices */
  d=LC_USBDevice_List_First(dl);
  while(d) {
    LC_USBDEVICE *dd;

    dd=LC_USBDevice_Find(um->currentDevices,
                         d->busId,
                         d->deviceId,
                         d->vendorId,
                         d->productId);
    if (!dd) {
      LC_USBDEVICE *newd;

      DBG_INFO(0, "Device %d/%d is new (%04x/%04x)",
               d->busId,
               d->deviceId,
               d->vendorId,
               d->productId);
      newd=LC_USBDevice_new(d->busId,
                            d->deviceId,
                            d->vendorId,
                            d->productId);
      newd->devicePos=d->devicePos;
      LC_USBDevice_List_Add(newd, um->newDevices);
    }
    d=LC_USBDevice_List_Next(d);
  }

  /* find lost devices */
  d=LC_USBDevice_List_First(um->currentDevices);
  while(d) {
    LC_USBDEVICE *dd;

    dd=LC_USBDevice_Find(dl,
                         d->busId,
                         d->deviceId,
                         d->vendorId,
                         d->productId);
    if (!dd) {
      LC_USBDEVICE *lostd;

      DBG_INFO(0, "Device %d/%d was lost (%04x/%04x)",
               d->busId,
               d->deviceId,
               d->vendorId,
               d->productId);
      lostd=LC_USBDevice_new(d->busId,
                             d->deviceId,
                             d->vendorId,
                             d->productId);
      lostd->devicePos=d->devicePos;
      LC_USBDevice_List_Add(lostd, um->lostDevices);
    }
    d=LC_USBDevice_List_Next(d);
  }

  LC_USBDevice_List_free(um->currentDevices);
  um->currentDevices=dl;
  return 0;
}



LC_USBDEVICE_LIST *LC_USBMonitor_GetNewDevices(const LC_USBMONITOR *um){
  assert(um);
  return um->newDevices;
}



LC_USBDEVICE_LIST *LC_USBMonitor_GetLostDevices(const LC_USBMONITOR *um){
  assert(um);
  return um->lostDevices;
}



LC_USBDEVICE_LIST *LC_USBMonitor_GetCurrentDevices(const LC_USBMONITOR *um){
  assert(um);
  return um->currentDevices;
}















