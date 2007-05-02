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
#include <gwenhywfar/misc.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/types.h>
#include <chipcard/chipcard.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LCM_SERVER, LCM_Server)
GWEN_LIST2_FUNCTIONS(LCM_SERVER, LCM_Server)



LCM_SERVER *LCM_Server_new(GWEN_TYPE_UINT32 serverId){
  LCM_SERVER *ms;

  GWEN_NEW_OBJECT(LCM_SERVER, ms);
  GWEN_LIST_INIT(LC_SERVER, ms);
  ms->serverId=serverId;
  ms->readers=LCM_Reader_List_new();
  ms->drivers=LCM_Driver_List_new();
  ms->services=LCM_Service_List_new();
  return ms;
}



void LCM_Server_free(LCM_SERVER *ms){
  if (ms) {
    free(ms->clientId);
    LCM_Service_List_free(ms->services);
    LCM_Reader_List_free(ms->readers);
    LCM_Driver_List_free(ms->drivers);
    GWEN_LIST_FINI(LCM_SERVER, ms);
    GWEN_FREE_OBJECT(ms);
  }
}



GWEN_TYPE_UINT32 LCM_Server_GetServerId(const LCM_SERVER *ms){
  assert(ms);
  return ms->serverId;
}



const char *LCM_Server_GetClientId(const LCM_SERVER *ms){
  assert(ms);
  return ms->clientId;
}



void LCM_Server_SetClientId(LCM_SERVER *ms, const char *s){
  assert(ms);
  free(ms->clientId);
  if (s) ms->clientId=strdup(s);
  else ms->clientId=0;
}



LCM_DRIVER_LIST *LCM_Server_GetDrivers(const LCM_SERVER *ms){
  assert(ms);
  return ms->drivers;
}



LCM_READER_LIST *LCM_Server_GetReaders(const LCM_SERVER *ms){
  assert(ms);
  return ms->readers;
}



LCM_SERVICE_LIST *LCM_Server_GetServices(const LCM_SERVER *ms){
  assert(ms);
  return ms->services;
}










