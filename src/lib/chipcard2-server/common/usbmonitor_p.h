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


#ifndef CHIPCARD_SERVER_USBMONITOR_P_H
#define CHIPCARD_SERVER_USBMONITOR_P_H


#define LC_USB_PROC_DIR "/proc/bus/usb"
#define LC_USB_PROC_BUS_USB_DEVICES_FILE "/proc/bus/usb/devices"

#include <chipcard2-server/common/usbmonitor.h>



struct LC_USBDEVICE {
  GWEN_LIST_ELEMENT(LC_USBDEVICE);
  GWEN_TYPE_UINT32 devicePos;
  GWEN_TYPE_UINT32 busId;
  GWEN_TYPE_UINT32 deviceId;
  GWEN_TYPE_UINT32 vendorId;
  GWEN_TYPE_UINT32 productId;
};


struct LC_USBMONITOR {
  LC_USBDEVICE_LIST *currentDevices;
  LC_USBDEVICE_LIST *newDevices;
  LC_USBDEVICE_LIST *lostDevices;


  GWEN_IDLIST *lastList;
};



int LC_USBMonitor_Read_UsbDevices(LC_USBDEVICE_LIST *dl);


#endif /* CHIPCARD_SERVER_USBMONITOR_P_H */
