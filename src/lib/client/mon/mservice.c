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

#include "service_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/types.h>
#include <chipcard/chipcard.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LCM_SERVICE, LCM_Service)
GWEN_LIST2_FUNCTIONS(LCM_SERVICE, LCM_Service)



LCM_SERVICE *LCM_Service_new(GWEN_TYPE_UINT32 serverId,
                             GWEN_TYPE_UINT32 serviceId,
                             const char *serviceName){
  LCM_SERVICE *ms;

  assert(serviceId);
  assert(serviceName);
  assert(serverId);

  GWEN_NEW_OBJECT(LCM_SERVICE, ms);
  GWEN_LIST_INIT(LC_SERVICE, ms);
  ms->serviceId=serviceId;
  ms->serviceName=strdup(serviceName);
  ms->serverId=serverId;
  ms->logBuffer=GWEN_Buffer_new(0, 512, 0, 1);

  return ms;
}



void LCM_Service_free(LCM_SERVICE *ms){
  if (ms) {
    free(ms->serviceName);
    GWEN_LIST_FINI(LCM_SERVICE, ms);
    GWEN_FREE_OBJECT(ms);
  }
}



GWEN_TYPE_UINT32 LCM_Service_GetServiceId(const LCM_SERVICE *ms){
  assert(ms);
  return ms->serviceId;
}



GWEN_TYPE_UINT32 LCM_Service_GetServerId(const LCM_SERVICE *ms){
  assert(ms);
  return ms->serverId;
}



const char *LCM_Service_GetServiceName(const LCM_SERVICE *ms){
  assert(ms);
  return ms->serviceName;
}




const char *LCM_Service_GetStatus(const LCM_SERVICE *ms){
  assert(ms);
  return ms->status;
}



void LCM_Service_SetStatus(LCM_SERVICE *ms, const char *s){
  assert(ms);
  free(ms->status);
  if (s) ms->status=strdup(s);
  else ms->status=0;
  ms->lastChangeTime=time(0);
}


GWEN_BUFFER *LCM_Service_GetLogBuffer(const LCM_SERVICE *ms){
  assert(ms);
  return ms->logBuffer;
}



time_t LCM_Service_GetLastChangeTime(const LCM_SERVICE *ms){
  assert(ms);
  return ms->lastChangeTime;
}


