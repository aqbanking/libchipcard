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


#include "serviceclient_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LC_SERVICECLIENT, LC_ServiceClient);
GWEN_INHERIT_FUNCTIONS(LC_SERVICECLIENT)


LC_SERVICECLIENT *LC_ServiceClient_new(uint32_t id){
  LC_SERVICECLIENT *cl;

  GWEN_NEW_OBJECT(LC_SERVICECLIENT, cl);
  GWEN_INHERIT_INIT(LC_SERVICECLIENT, cl);
  GWEN_LIST_INIT(LC_SERVICECLIENT, cl);

  /* assign unique id */
  cl->clientId=id;

  return cl;
}



void LC_ServiceClient_free(LC_SERVICECLIENT *cl){
  if (cl) {
    GWEN_INHERIT_FINI(LC_SERVICECLIENT, cl);
    GWEN_LIST_FINI(LC_SERVICECLIENT, cl);
    free(cl->userName);
    GWEN_FREE_OBJECT(cl);
  }
}



const char *LC_ServiceClient_GetUserName(const LC_SERVICECLIENT *cl){
  assert(cl);
  return cl->userName;
}



void LC_ServiceClient_SetUserName(LC_SERVICECLIENT *cl, const char *s){
  assert(cl);
  free(cl->userName);
  if (s)
    cl->userName=strdup(s);
  else
    cl->userName=0;
}



uint32_t LC_ServiceClient_GetClientId(const LC_SERVICECLIENT *cl){
  assert(cl);
  return cl->clientId;
}






