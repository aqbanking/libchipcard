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


GWEN_LIST_FUNCTIONS(LC_USBDEVICE, LC_USBDevice)



LC_USBDEVICE *LC_USBDevice_new(GWEN_TYPE_UINT32 busId,
                               GWEN_TYPE_UINT32 deviceId,
                               GWEN_TYPE_UINT32 vendorId,
                               GWEN_TYPE_UINT32 productId) {
  LC_USBDEVICE *ud;

  GWEN_NEW_OBJECT(LC_USBDEVICE, ud);
  DBG_MEM_INC("LC_USBDEVICE", 0);
  GWEN_LIST_INIT(LC_USBDEVICE, ud);

  ud->busId=busId;
  ud->deviceId=deviceId;
  ud->vendorId=vendorId;
  ud->productId=productId;

  return ud;
}



void LC_USBDevice_free(LC_USBDEVICE *ud) {
  if (ud) {
    GWEN_LIST_FINI(LC_USBDEVICE, ud);
    GWEN_FREE_OBJECT(ud);
    DBG_MEM_DEC("LC_USBDEVICE");
  }
}



GWEN_TYPE_UINT32 LC_USBDevice_GetDevicePos(const LC_USBDEVICE *ud){
  assert(ud);
  return ud->devicePos;
}




GWEN_TYPE_UINT32 LC_USBDevice_GetBusId(const LC_USBDEVICE *ud){
  assert(ud);
  return ud->busId;
}



GWEN_TYPE_UINT32 LC_USBDevice_GetDeviceId(const LC_USBDEVICE *ud){
  assert(ud);
  return ud->deviceId;
}



GWEN_TYPE_UINT32 LC_USBDevice_GetVendorId(const LC_USBDEVICE *ud){
  assert(ud);
  return ud->vendorId;
}



GWEN_TYPE_UINT32 LC_USBDevice_GetProductId(const LC_USBDEVICE *ud){
  assert(ud);
  return ud->productId;
}





LC_USBMONITOR *LC_USBMonitor_new() {
  LC_USBMONITOR *um;

  GWEN_NEW_OBJECT(LC_USBMONITOR, um);
  DBG_MEM_INC("LC_USBMONITOR", 0);
  um->currentDevices=LC_USBDevice_List_new();
  um->newDevices=LC_USBDevice_List_new();
  um->lostDevices=LC_USBDevice_List_new();

#ifdef USE_LIBUSB
  usb_init();
#endif

  return um;
}



void LC_USBMonitor_free(LC_USBMONITOR *um) {
  if (um) {
    GWEN_IdList_free(um->lastList);

    LC_USBDevice_List_free(um->currentDevices);
    LC_USBDevice_List_free(um->newDevices);
    LC_USBDevice_List_free(um->lostDevices);

    GWEN_FREE_OBJECT(um);
    DBG_MEM_DEC("LC_USBMONITOR");
  }
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
  return 0;
#else
  DBG_DEBUG(0, "No USB support");
  return -1;
#endif
}



LC_USBDEVICE *LC_USBDevice_Find(LC_USBDEVICE_LIST *dl,
                                GWEN_TYPE_UINT32 busId,
                                GWEN_TYPE_UINT32 deviceId,
                                GWEN_TYPE_UINT32 vendorId,
                                GWEN_TYPE_UINT32 productId) {
  LC_USBDEVICE *d;

  d=LC_USBDevice_List_First(dl);
  while(d) {
    if ((busId==0 || busId==d->busId) &&
        (deviceId==0 || deviceId==d->deviceId) &&
        (vendorId==0 || vendorId==d->vendorId) &&
        (productId==0 || productId==d->productId))
      return d;
    d=LC_USBDevice_List_Next(d);
  } /* while */

  return 0;
}



LC_USBDEVICE *LC_USBDevice_Get(LC_USBDEVICE_LIST *dl,
                               GWEN_TYPE_UINT32 dpos) {
  LC_USBDEVICE *d;

  d=LC_USBDevice_List_First(dl);
  while(d) {
    if (dpos==d->devicePos)
      return d;
    d=LC_USBDevice_List_Next(d);
  } /* while */

  return 0;
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















