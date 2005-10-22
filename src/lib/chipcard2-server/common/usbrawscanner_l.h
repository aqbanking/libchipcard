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


#ifndef CHIPCARD_SERVER_USBSCANNER_L_H
#define CHIPCARD_SERVER_USBSCANNER_L_H

#define LC_USB_DEVICE_FILE "/var/run/chipcard2/usb.state"

#include "devmonitor.h"


LC_DEVSCANNER *LC_UsbRawScanner_new();





#endif /* CHIPCARD_SERVER_USBSCANNER_L_H */
