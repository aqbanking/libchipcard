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


#ifndef CHIPCARD_CLIENT_NOTIFICATIONS_L_H
#define CHIPCARD_CLIENT_NOTIFICATIONS_L_H

#include <chipcard2-client/client/notifications.h>

GWEN_LIST_FUNCTION_DEFS(LC_NOTIFICATION, LC_Notification)

LC_NOTIFICATION *LC_Notification_new(GWEN_TYPE_UINT32 serverId,
                                     const char *clientId,
                                     const char *ntype,
                                     const char *ncode,
                                     GWEN_DB_NODE *data);



#endif /* CHIPCARD_CLIENT_NOTIFICATIONS_L_H */

