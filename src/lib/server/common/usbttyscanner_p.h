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


#ifndef CHIPCARD_SERVER_USBTTYSCANNER_P_H
#define CHIPCARD_SERVER_USBTTYSCANNER_P_H

#include "usbttyscanner_l.h"


static int LC_UsbTtyScanner_Read_ProcTtyDriverUsbSerial(LC_DEVICE_LIST *dl);

static int LC_UsbTtyScanner_ScanSysFS_UsbSerial(LC_DEVICE_LIST *dl);

static int LC_UsbTtyScanner_ReadDevs(LC_DEVSCANNER *sc, LC_DEVICE_LIST *dl);


#endif /* CHIPCARD_SERVER_USBTTYMONITOR_P_H */
