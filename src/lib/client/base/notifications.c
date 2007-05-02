/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: notifications.c 23 2005-01-24 23:54:15Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "notifications_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>
#include <chipcard3/chipcard3.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <time.h>


GWEN_LIST_FUNCTIONS(LC_NOTIFICATION, LC_Notification)
GWEN_LIST2_FUNCTIONS(LC_NOTIFICATION, LC_Notification)
GWEN_INHERIT_FUNCTIONS(LC_NOTIFICATION)


LC_NOTIFICATION *LC_Notification_new(GWEN_TYPE_UINT32 serverId,
                                     const char *clientId,
                                     const char *ntype,
                                     const char *ncode,
                                     GWEN_DB_NODE *data){
  LC_NOTIFICATION *n;

  assert(clientId);
  assert(ntype);
  assert(ncode);
  GWEN_NEW_OBJECT(LC_NOTIFICATION, n);
  n->serverId=serverId;
  n->clientId=strdup(clientId);
  n->ntype=strdup(ntype);
  n->ncode=strdup(ncode);
  if (data)
    n->data=GWEN_DB_Group_dup(data);

  return n;
}



void LC_Notification_free(LC_NOTIFICATION *n){
  if (n) {
    free(n->ntype);
    free(n->ncode);
    free(n->clientId);
    GWEN_DB_Group_free(n->data);

    GWEN_FREE_OBJECT(n);
  }
}



GWEN_TYPE_UINT32 LC_Notification_GetServerId(const LC_NOTIFICATION *n){
  assert(n);
  return n->serverId;
}



const char *LC_Notification_GetClientId(const LC_NOTIFICATION *n){
  assert(n);
  return n->clientId;
}



const char *LC_Notification_GetType(const LC_NOTIFICATION *n){
  assert(n);
  return n->ntype;
}



const char *LC_Notification_GetCode(const LC_NOTIFICATION *n){
  assert(n);
  return n->ncode;
}



GWEN_DB_NODE *LC_Notification_GetData(const LC_NOTIFICATION *n){
  assert(n);
  return n->data;
}
















