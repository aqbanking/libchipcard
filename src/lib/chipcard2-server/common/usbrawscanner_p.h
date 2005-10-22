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

#include "usbrawscanner_l.h"


int LC_UsbRawScanner_ReadDevs(LC_DEVSCANNER *sc, LC_DEVICE_LIST *dl);


#endif /* CHIPCARD_SERVER_USBMONITOR_P_H */
