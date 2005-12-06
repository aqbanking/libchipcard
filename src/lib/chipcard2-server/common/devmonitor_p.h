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


#ifndef CHIPCARD_SERVER_DEVMONITOR_P_H
#define CHIPCARD_SERVER_DEVMONITOR_P_H


#include "devmonitor.h"



struct LC_DEVICE {
  GWEN_LIST_ELEMENT(LC_DEVICE);
  char *path;
  LC_DEVICE_BUSTYPE busType;
  GWEN_TYPE_UINT32 devicePos;
  GWEN_TYPE_UINT32 busId;
  GWEN_TYPE_UINT32 deviceId;
  GWEN_TYPE_UINT32 vendorId;
  GWEN_TYPE_UINT32 productId;
  char *readerType;
  char *driverType;
};


struct LC_DEVSCANNER {
  GWEN_INHERIT_ELEMENT(LC_DEVSCANNER);
  GWEN_LIST_ELEMENT(LC_DEVSCANNER);
  LC_DEVSCANNER_READ_DEVS_FN readDevsFn;
};


int LC_DevScanner_ReadDevs(LC_DEVSCANNER *um, LC_DEVICE_LIST *dl);


struct LC_DEVMONITOR {
  LC_DEVSCANNER_LIST *scanners;

  LC_DEVICE_LIST *currentDevices;
  LC_DEVICE_LIST *newDevices;
  LC_DEVICE_LIST *lostDevices;
};



#endif /* CHIPCARD_SERVER_DEVMONITOR_P_H */
