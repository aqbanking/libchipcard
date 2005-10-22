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


#include "devmonitor_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/directory.h>
#include <gwenhywfar/buffer.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>



GWEN_LIST_FUNCTIONS(LC_DEVICE, LC_Device)
GWEN_LIST_FUNCTIONS(LC_DEVSCANNER, LC_DevScanner)
GWEN_INHERIT_FUNCTIONS(LC_DEVSCANNER)



LC_DEVICE_BUSTYPE LC_Device_BusType_fromString(const char *s) {
  if (strcasecmp(s, "any")==0)
    return LC_Device_BusType_Any;
  else if (strcasecmp(s, "UsbRaw")==0)
    return LC_Device_BusType_UsbRaw;
  else if (strcasecmp(s, "UsbTty")==0)
    return LC_Device_BusType_UsbTty;
  else if (strcasecmp(s, "pci")==0)
    return LC_Device_BusType_Pci;
  else if (strcasecmp(s, "pcmcia")==0)
    return LC_Device_BusType_Pcmcia;
  else if (strcasecmp(s, "serial")==0)
    return LC_Device_BusType_Serial;
  return LC_Device_BusType_Unknown;
}



const char *LC_Device_BusType_toString(LC_DEVICE_BUSTYPE i) {
  switch(i) {
  case LC_Device_BusType_Any:    return "any";
  case LC_Device_BusType_UsbRaw: return "UsbRaw";
  case LC_Device_BusType_UsbTty: return "UsbTty";
  case LC_Device_BusType_Pci:    return "pci";
  case LC_Device_BusType_Pcmcia: return "pcmcia";
  case LC_Device_BusType_Serial: return "serial";
  default:                       return "unknown";
  }
}




LC_DEVICE *LC_Device_new(LC_DEVICE_BUSTYPE busType,
                         GWEN_TYPE_UINT32 busId,
                         GWEN_TYPE_UINT32 deviceId,
                         GWEN_TYPE_UINT32 vendorId,
                         GWEN_TYPE_UINT32 productId) {
  LC_DEVICE *ud;

  GWEN_NEW_OBJECT(LC_DEVICE, ud);
  DBG_MEM_INC("LC_DEVICE", 0);
  GWEN_LIST_INIT(LC_DEVICE, ud);

  ud->busType=busType;
  ud->busId=busId;
  ud->deviceId=deviceId;
  ud->vendorId=vendorId;
  ud->productId=productId;

  return ud;
}



void LC_Device_free(LC_DEVICE *ud) {
  if (ud) {
    GWEN_LIST_FINI(LC_DEVICE, ud);
    free(ud->path);
    GWEN_FREE_OBJECT(ud);
    DBG_MEM_DEC("LC_DEVICE");
  }
}



LC_DEVICE_BUSTYPE LC_Device_GetBusType(const LC_DEVICE *ud) {
  assert(ud);
  return ud->busType;
}



GWEN_TYPE_UINT32 LC_Device_GetDevicePos(const LC_DEVICE *ud){
  assert(ud);
  return ud->devicePos;
}



void LC_Device_SetDevicePos(LC_DEVICE *ud, GWEN_TYPE_UINT32 i) {
  assert(ud);
  ud->devicePos=i;
}




GWEN_TYPE_UINT32 LC_Device_GetBusId(const LC_DEVICE *ud){
  assert(ud);
  return ud->busId;
}



GWEN_TYPE_UINT32 LC_Device_GetDeviceId(const LC_DEVICE *ud){
  assert(ud);
  return ud->deviceId;
}



GWEN_TYPE_UINT32 LC_Device_GetVendorId(const LC_DEVICE *ud){
  assert(ud);
  return ud->vendorId;
}



GWEN_TYPE_UINT32 LC_Device_GetProductId(const LC_DEVICE *ud){
  assert(ud);
  return ud->productId;
}



const char *LC_Device_GetPath(const LC_DEVICE *ud) {
  assert(ud);
  return ud->path;
}



void LC_Device_SetPath(LC_DEVICE *ud, const char *s) {
  assert(ud);
  free(ud->path);
  if (s) ud->path=strdup(s);
  else ud->path=0;
}



LC_DEVICE *LC_Device_List_Find(LC_DEVICE_LIST *dl,
                               LC_DEVICE_BUSTYPE busType,
                               GWEN_TYPE_UINT32 busId,
                               GWEN_TYPE_UINT32 deviceId,
                               GWEN_TYPE_UINT32 vendorId,
                               GWEN_TYPE_UINT32 productId) {
  LC_DEVICE *d;

  d=LC_Device_List_First(dl);
  while(d) {
    if ((busType==LC_Device_BusType_Any || busType==d->busType) &&
	(busId==0 || busId==d->busId) &&
        (deviceId==0 || deviceId==d->deviceId) &&
        (vendorId==0 || vendorId==d->vendorId) &&
        (productId==0 || productId==d->productId))
      return d;
    d=LC_Device_List_Next(d);
  } /* while */

  return 0;
}



LC_DEVICE *LC_Device_Get(LC_DEVICE_LIST *dl,
			 LC_DEVICE_BUSTYPE busType,
			 GWEN_TYPE_UINT32 dpos) {
  LC_DEVICE *d;

  d=LC_Device_List_First(dl);
  while(d) {
    if ((d->busType==busType) &&
	(dpos==d->devicePos))
      return d;
    d=LC_Device_List_Next(d);
  } /* while */

  return 0;
}







LC_DEVSCANNER *LC_DevScanner_new() {
  LC_DEVSCANNER *um;

  GWEN_NEW_OBJECT(LC_DEVSCANNER, um);
  DBG_MEM_INC("LC_DEVSCANNER", 0);
  GWEN_INHERIT_INIT(LC_DEVSCANNER, um);
  GWEN_LIST_INIT(LC_DEVSCANNER, um);

  return um;
}



void LC_DevScanner_free(LC_DEVSCANNER *um) {
  if (um) {
    GWEN_LIST_FINI(LC_DEVSCANNER, um);
    GWEN_INHERIT_FINI(LC_DEVSCANNER, um);
    GWEN_FREE_OBJECT(um);
    DBG_MEM_DEC("LC_DEVSCANNER");
  }
}




int LC_DevScanner_ReadDevs(LC_DEVSCANNER *um, LC_DEVICE_LIST *dl) {
  assert(um);
  assert(um->readDevsFn);
  return um->readDevsFn(um, dl);
}



void LC_DevScanner_SetReadDevsFn(LC_DEVSCANNER *um,
                                 LC_DEVSCANNER_READ_DEVS_FN fn) {
  assert(um);
  um->readDevsFn=fn;
}












LC_DEVMONITOR *LC_DevMonitor_new() {
  LC_DEVMONITOR *um;

  GWEN_NEW_OBJECT(LC_DEVMONITOR, um);
  DBG_MEM_INC("LC_DEVMONITOR", 0);
  um->currentDevices=LC_Device_List_new();
  um->newDevices=LC_Device_List_new();
  um->lostDevices=LC_Device_List_new();
  um->scanners=LC_DevScanner_List_new();
  return um;
}



void LC_DevMonitor_free(LC_DEVMONITOR *um) {
  if (um) {
    LC_DevScanner_List_free(um->scanners);
    LC_Device_List_free(um->currentDevices);
    LC_Device_List_free(um->newDevices);
    LC_Device_List_free(um->lostDevices);

    GWEN_FREE_OBJECT(um);
    DBG_MEM_DEC("LC_DEVMONITOR");
  }
}



int LC_DevMonitor_Scan(LC_DEVMONITOR *um) {
  LC_DEVICE_LIST *dl;
  LC_DEVICE *d;
  LC_DEVSCANNER *scanner;
  int oks=0;
  int changes=0;

  LC_Device_List_Clear(um->newDevices);
  LC_Device_List_Clear(um->lostDevices);

  dl=LC_Device_List_new();

  scanner=LC_DevScanner_List_First(um->scanners);
  while(scanner) {
    int rv;

    rv=LC_DevScanner_ReadDevs(scanner, dl);
    if (rv==-1) {
      DBG_INFO(0, "here");
    }
    else if (rv==0)
      oks++;
    scanner=LC_DevScanner_List_Next(scanner);
  }

  if (oks==0) {
    DBG_INFO(0, "No scanner succeeded");
    LC_Device_List_free(dl);
    return -1;
  }

  /* find new devices */
  d=LC_Device_List_First(dl);
  while(d) {
    LC_DEVICE *dd;

    dd=LC_Device_List_Find(um->currentDevices,
			   d->busType,
			   d->busId,
			   d->deviceId,
			   d->vendorId,
			   d->productId);
    if (!dd) {
      LC_DEVICE *newd;

      DBG_DEBUG(0, "Device %s/%d/%d is new (%04x/%04x)",
                LC_Device_BusType_toString(LC_Device_GetBusType(d)),
                d->busId,
                d->deviceId,
                d->vendorId,
                d->productId);
      newd=LC_Device_new(d->busType,
			 d->busId,
			 d->deviceId,
			 d->vendorId,
			 d->productId);
      newd->devicePos=d->devicePos;
      LC_Device_List_Add(newd, um->newDevices);
      changes++;
    }
    d=LC_Device_List_Next(d);
  }

  /* find lost devices */
  d=LC_Device_List_First(um->currentDevices);
  while(d) {
    LC_DEVICE *dd;

    dd=LC_Device_List_Find(dl,
			   d->busType,
			   d->busId,
			   d->deviceId,
			   d->vendorId,
			   d->productId);
    if (!dd) {
      LC_DEVICE *lostd;

      DBG_DEBUG(0, "Device %s/%d/%d was lost (%04x/%04x)",
                LC_Device_BusType_toString(LC_Device_GetBusType(d)),
                d->busId,
                d->deviceId,
                d->vendorId,
                d->productId);
      lostd=LC_Device_new(d->busType,
			  d->busId,
			  d->deviceId,
			  d->vendorId,
			  d->productId);
      lostd->devicePos=d->devicePos;
      LC_Device_List_Add(lostd, um->lostDevices);
      changes++;
    }
    d=LC_Device_List_Next(d);
  }

  LC_Device_List_free(um->currentDevices);
  um->currentDevices=dl;
  if (changes)
    return 0;
  return 1;
}



LC_DEVICE_LIST *LC_DevMonitor_GetNewDevices(const LC_DEVMONITOR *um){
  assert(um);
  return um->newDevices;
}



LC_DEVICE_LIST *LC_DevMonitor_GetLostDevices(const LC_DEVMONITOR *um){
  assert(um);
  return um->lostDevices;
}



LC_DEVICE_LIST *LC_DevMonitor_GetCurrentDevices(const LC_DEVMONITOR *um){
  assert(um);
  return um->currentDevices;
}



void LC_DevMonitor_AddScanner(LC_DEVMONITOR *um, LC_DEVSCANNER *sc) {
  assert(um);
  assert(sc);
  LC_DevScanner_List_Add(sc, um->scanners);
}
















