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


#include <chipcard2-server/common/devmonitor.h>



struct LC_DEVICE {
  GWEN_LIST_ELEMENT(LC_DEVICE);
  LC_DEVICE_BUSTYPE busType;
  GWEN_TYPE_UINT32 devicePos;
  GWEN_TYPE_UINT32 busId;
  GWEN_TYPE_UINT32 deviceId;
  GWEN_TYPE_UINT32 vendorId;
  GWEN_TYPE_UINT32 productId;
};


struct LC_DEVMONITOR {
  GWEN_INHERIT_ELEMENT(LC_DEVMONITOR);
  LC_DEVICE_LIST *currentDevices;
  LC_DEVICE_LIST *newDevices;
  LC_DEVICE_LIST *lostDevices;

  GWEN_IDLIST *lastList;

  LC_DEVMONITOR_READ_DEVS_FN readDevsFn;

};



int LC_DevMonitor_ReadDevs(LC_DEVMONITOR *um, LC_DEVICE_LIST *dl);



#endif /* CHIPCARD_SERVER_DEVMONITOR_P_H */
