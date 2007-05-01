/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: server.h 2 2005-01-02 10:05:37Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CLIENT_SERVER_L_H
#define CHIPCARD_CLIENT_SERVER_L_H

#include <gwenhywfar/misc.h>

typedef enum {
  LC_ServerStatusUnconnected=0,
  LC_ServerStatusWaitReady,
  LC_ServerStatusConnected,
  LC_ServerStatusAborted
} LC_SERVER_STATUS;

typedef struct LC_SERVER LC_SERVER;
GWEN_LIST_FUNCTION_DEFS(LC_SERVER, LC_Server);

LC_SERVER *LC_Server_new(GWEN_TYPE_UINT32 nid);
void LC_Server_free(LC_SERVER *sv);

GWEN_TYPE_UINT32 LC_Server_GetCurrentCommand(const LC_SERVER *sv);
void LC_Server_SetCurrentCommand(LC_SERVER *sv,
                                 GWEN_TYPE_UINT32 rid);

GWEN_TYPE_UINT32 LC_Server_GetServerId(const LC_SERVER *sv);

LC_SERVER_STATUS LC_Server_GetStatus(const LC_SERVER *sv);
void LC_Server_SetStatus(LC_SERVER *sv, LC_SERVER_STATUS st);


#endif /* CHIPCARD_CLIENT_SERVER_L_H */
