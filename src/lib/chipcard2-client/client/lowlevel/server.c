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


#include "server_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <chipcard2-client/chipcard2.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>



GWEN_LIST_FUNCTIONS(LC_SERVER, LC_Server);



LC_SERVER *LC_Server_new(GWEN_TYPE_UINT32 nid){
  LC_SERVER *sv;

  GWEN_NEW_OBJECT(LC_SERVER, sv);
  GWEN_LIST_INIT(LC_SERVER, sv);
  sv->serverId=nid;
  return sv;
}



void LC_Server_free(LC_SERVER *sv){
  if (sv) {
    GWEN_LIST_FINI(LC_SERVER, sv);
    GWEN_FREE_OBJECT(sv);
  }
}



LC_SERVER_STATUS LC_Server_GetStatus(const LC_SERVER *sv){
  assert(sv);
  return sv->status;
}



void LC_Server_SetStatus(LC_SERVER *sv, LC_SERVER_STATUS st){
  assert(sv);
  sv->status=st;
}



GWEN_TYPE_UINT32 LC_Server_GetServerId(const LC_SERVER *sv){
  assert(sv);
  return sv->serverId;
}



GWEN_TYPE_UINT32 LC_Server_GetCurrentCommand(const LC_SERVER *sv){
  assert(sv);
  return sv->currentCommand;
}



void LC_Server_SetCurrentCommand(LC_SERVER *sv,
                                 GWEN_TYPE_UINT32 rid){
  assert(sv);
  sv->currentCommand=rid;
}

















