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


#ifndef CHIPCARD_SERVER_USBTTYMONITOR_P_H
#define CHIPCARD_SERVER_USBTTYMONITOR_P_H


#define LC_USBTTY_PROC_TTY_DRIVER_USBSERIAL_FILE \
  "/proc/tty/driver/usb-serial"
#define LC_USBTTY_PROC_TTY_DRIVER_USBSERIAL2_6_FILE \
  "/proc/tty/driver/usbserial"

#include <chipcard2-server/common/usbttymonitor.h>



struct LC_USBTTYDEVICE {
  GWEN_LIST_ELEMENT(LC_USBTTYDEVICE);
  GWEN_TYPE_UINT32 port;
  GWEN_TYPE_UINT32 vendorId;
  GWEN_TYPE_UINT32 productId;
};


struct LC_USBTTYMONITOR {
  LC_USBTTYDEVICE_LIST *currentDevices;
  LC_USBTTYDEVICE_LIST *newDevices;
  LC_USBTTYDEVICE_LIST *lostDevices;

  GWEN_IDLIST *lastList;
};



LC_USBTTYDEVICE *LC_USBTTYDevice_Find(LC_USBTTYDEVICE_LIST *dl,
                                      GWEN_TYPE_UINT32 port,
                                      GWEN_TYPE_UINT32 vendorId,
                                      GWEN_TYPE_UINT32 productId);


int LC_USBTTYMonitor_Read_ProcTtyDriverUsbSerial(LC_USBTTYDEVICE_LIST *dl);

int LC_USBTTYMonitor_ScanSysFS_UsbSerial(LC_USBTTYDEVICE_LIST *dl);


#endif /* CHIPCARD_SERVER_USBTTYMONITOR_P_H */
