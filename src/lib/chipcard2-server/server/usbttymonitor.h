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


#ifndef CHIPCARD_SERVER_USBTTYMONITOR_H
#define CHIPCARD_SERVER_USBTTYMONITOR_H


typedef struct LC_USBTTYMONITOR LC_USBTTYMONITOR;
typedef struct LC_USBTTYDEVICE LC_USBTTYDEVICE;

#include <gwenhywfar/idlist.h>
#include <gwenhywfar/misc.h>


GWEN_LIST_FUNCTION_DEFS(LC_USBTTYDEVICE, LC_USBTTYDevice)


LC_USBTTYMONITOR *LC_USBTTYMonitor_new();
void LC_USBTTYMonitor_free(LC_USBTTYMONITOR *um);



LC_USBTTYDEVICE *LC_USBTTYDevice_new(GWEN_TYPE_UINT32 port,
                                     GWEN_TYPE_UINT32 vendorId,
                                     GWEN_TYPE_UINT32 productId);
void LC_USBTTYDevice_free(LC_USBTTYDEVICE *ud);


int LC_USBTTYMonitor_Scan(LC_USBTTYMONITOR *um);

LC_USBTTYDEVICE_LIST *LC_USBTTYMonitor_GetNewDevices(const LC_USBTTYMONITOR *um);
LC_USBTTYDEVICE_LIST *LC_USBTTYMonitor_GetLostDevices(const LC_USBTTYMONITOR *um);



GWEN_TYPE_UINT32 LC_USBTTYDevice_GetPort(const LC_USBTTYDEVICE *ud);
GWEN_TYPE_UINT32 LC_USBTTYDevice_GetVendorId(const LC_USBTTYDEVICE *ud);
GWEN_TYPE_UINT32 LC_USBTTYDevice_GetProductId(const LC_USBTTYDEVICE *ud);




#endif /* CHIPCARD_SERVER_USBTTYMONITOR_H */
