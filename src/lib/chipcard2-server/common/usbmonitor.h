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


#ifndef CHIPCARD_SERVER_USBMONITOR_H
#define CHIPCARD_SERVER_USBMONITOR_H

#define LC_USB_DEVICE_FILE "/var/run/chipcard2/usb.state"


typedef struct LC_USBMONITOR LC_USBMONITOR;
typedef struct LC_USBDEVICE LC_USBDEVICE;

#include <gwenhywfar/idlist.h>
#include <gwenhywfar/misc.h>


GWEN_LIST_FUNCTION_DEFS(LC_USBDEVICE, LC_USBDevice)


LC_USBMONITOR *LC_USBMonitor_new();
void LC_USBMonitor_free(LC_USBMONITOR *um);

int LC_USBMonitor_Scan(LC_USBMONITOR *um);

LC_USBDEVICE *LC_USBDevice_new(GWEN_TYPE_UINT32 busId,
                               GWEN_TYPE_UINT32 deviceId,
                               GWEN_TYPE_UINT32 vendorId,
                               GWEN_TYPE_UINT32 productId);
void LC_USBDevice_free(LC_USBDEVICE *ud);

LC_USBDEVICE *LC_USBDevice_Find(LC_USBDEVICE_LIST *dl,
                                GWEN_TYPE_UINT32 busId,
                                GWEN_TYPE_UINT32 deviceId,
                                GWEN_TYPE_UINT32 vendorId,
                                GWEN_TYPE_UINT32 productId);

LC_USBDEVICE *LC_USBDevice_Get(LC_USBDEVICE_LIST *dl,
                               GWEN_TYPE_UINT32 dpos);


LC_USBDEVICE_LIST *LC_USBMonitor_GetNewDevices(const LC_USBMONITOR *um);
LC_USBDEVICE_LIST *LC_USBMonitor_GetLostDevices(const LC_USBMONITOR *um);
LC_USBDEVICE_LIST *LC_USBMonitor_GetCurrentDevices(const LC_USBMONITOR *um);



GWEN_TYPE_UINT32 LC_USBDevice_GetDevicePos(const LC_USBDEVICE *ud);
GWEN_TYPE_UINT32 LC_USBDevice_GetBusId(const LC_USBDEVICE *ud);
GWEN_TYPE_UINT32 LC_USBDevice_GetDeviceId(const LC_USBDEVICE *ud);
GWEN_TYPE_UINT32 LC_USBDevice_GetVendorId(const LC_USBDEVICE *ud);
GWEN_TYPE_UINT32 LC_USBDevice_GetProductId(const LC_USBDEVICE *ud);




#endif /* CHIPCARD_SERVER_USBMONITOR_H */
