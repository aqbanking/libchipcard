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


#ifndef CHIPCARD_CLIENT_NOTIFICATIONS_H
#define CHIPCARD_CLIENT_NOTIFICATIONS_H

#include <gwenhywfar/types.h>
#include <gwenhywfar/db.h>
#include <gwenhywfar/list2.h>


typedef struct LC_NOTIFICATION LC_NOTIFICATION;

GWEN_LIST2_FUNCTION_DEFS(LC_NOTIFICATION, LC_Notification)
GWEN_INHERIT_FUNCTION_DEFS(LC_NOTIFICATION)

void LC_Notification_free(LC_NOTIFICATION *n);

GWEN_TYPE_UINT32 LC_Notification_GetServerId(const LC_NOTIFICATION *n);
const char *LC_Notification_GetClientId(const LC_NOTIFICATION *n);
const char *LC_Notification_GetType(const LC_NOTIFICATION *n);
const char *LC_Notification_GetCode(const LC_NOTIFICATION *n);
GWEN_DB_NODE *LC_Notification_GetData(const LC_NOTIFICATION *n);




#endif /* CHIPCARD_CLIENT_NOTIFICATIONS_H */

