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


#ifndef CHIPCARD_SERVER_DEVMONITOR_H
#define CHIPCARD_SERVER_DEVMONITOR_H


typedef struct LC_DEVICE LC_DEVICE;
typedef struct LC_DEVSCANNER LC_DEVSCANNER;
typedef struct LC_DEVMONITOR LC_DEVMONITOR;


#include <gwenhywfar/idlist.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/buffer.h>


typedef enum {
  LC_Device_BusType_Unknown=-1,
  LC_Device_BusType_Any=0,
  LC_Device_BusType_UsbRaw,
  LC_Device_BusType_UsbTty,
  LC_Device_BusType_Pci,
  LC_Device_BusType_Pcmcia,
  LC_Device_BusType_Serial
} LC_DEVICE_BUSTYPE;

LC_DEVICE_BUSTYPE LC_Device_BusType_fromString(const char *s);
const char *LC_Device_BusType_toString(LC_DEVICE_BUSTYPE i);


GWEN_LIST_FUNCTION_DEFS(LC_DEVICE, LC_Device)
GWEN_INHERIT_FUNCTION_DEFS(LC_DEVSCANNER)
GWEN_LIST_FUNCTION_DEFS(LC_DEVSCANNER, LC_DevScanner)


typedef int (*LC_DEVSCANNER_READ_DEVS_FN)(LC_DEVSCANNER *um,
                                          LC_DEVICE_LIST *dl);


LC_DEVSCANNER *LC_DevScanner_new();
void LC_DevScanner_SetReadDevsFn(LC_DEVSCANNER *um,
                                 LC_DEVSCANNER_READ_DEVS_FN fn);

void LC_DevScanner_free(LC_DEVSCANNER *um);

int LC_DevScanner_Scan(LC_DEVSCANNER *um,
                       LC_DEVICE_LIST *devList);


LC_DEVMONITOR *LC_DevMonitor_new();
void LC_DevMonitor_free(LC_DEVMONITOR *um);

void LC_DevMonitor_AddScanner(LC_DEVMONITOR *um, LC_DEVSCANNER *sc);

int LC_DevMonitor_Scan(LC_DEVMONITOR *um);

LC_DEVICE_LIST *LC_DevMonitor_GetNewDevices(const LC_DEVMONITOR *um);
LC_DEVICE_LIST *LC_DevMonitor_GetLostDevices(const LC_DEVMONITOR *um);
LC_DEVICE_LIST *LC_DevMonitor_GetCurrentDevices(const LC_DEVMONITOR *um);




LC_DEVICE *LC_Device_new(LC_DEVICE_BUSTYPE busType,
                         uint32_t busId,
                         uint32_t deviceId,
                         uint32_t vendorId,
                         uint32_t productId);
void LC_Device_free(LC_DEVICE *ud);
LC_DEVICE *LC_Device_dup(const LC_DEVICE *od);


LC_DEVICE *LC_Device_List_Find(LC_DEVICE_LIST *dl,
                               LC_DEVICE_BUSTYPE busType,
                               uint32_t busId,
                               uint32_t deviceId,
                               uint32_t vendorId,
                               uint32_t productId);

LC_DEVICE *LC_Device_Get(LC_DEVICE_LIST *dl,
                         LC_DEVICE_BUSTYPE busType,
                         uint32_t dpos);


uint32_t LC_Device_GetDevicePos(const LC_DEVICE *ud);
void LC_Device_SetDevicePos(LC_DEVICE *ud, uint32_t i);

const char *LC_Device_GetPath(const LC_DEVICE *ud);
void LC_Device_SetPath(LC_DEVICE *ud, const char *s);

LC_DEVICE_BUSTYPE LC_Device_GetBusType(const LC_DEVICE *ud);
uint32_t LC_Device_GetBusId(const LC_DEVICE *ud);
uint32_t LC_Device_GetDeviceId(const LC_DEVICE *ud);
uint32_t LC_Device_GetVendorId(const LC_DEVICE *ud);
uint32_t LC_Device_GetProductId(const LC_DEVICE *ud);

const char *LC_Device_GetBusName(const LC_DEVICE *ud);
void LC_Device_SetBusName(LC_DEVICE *ud, const char *s);

const char *LC_Device_GetDeviceName(const LC_DEVICE *ud);
void LC_Device_SetDeviceName(LC_DEVICE *ud, const char *s);

const char *LC_Device_GetDriverType(const LC_DEVICE *ud);
void LC_Device_SetDriverType(LC_DEVICE *ud, const char *s);

const char *LC_Device_GetReaderType(const LC_DEVICE *ud);
void LC_Device_SetReaderType(LC_DEVICE *ud, const char *s);

int LC_Device_ReplaceVars(const LC_DEVICE *d, const char *tmpl,
                          GWEN_BUFFER *buf);


#endif /* CHIPCARD_SERVER_DEVMONITOR_H */
