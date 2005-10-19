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


#include "cl_client_p.h"
#include <chipcard2/chipcard2.h>

#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LCCL_CLIENT, LCCL_Client);


LCCL_CLIENT *LCCL_Client_new(GWEN_TYPE_UINT32 id){
  LCCL_CLIENT *cl;

  GWEN_NEW_OBJECT(LCCL_CLIENT, cl);
  DBG_MEM_INC("LCCL_CLIENT", 0);
  cl->usage=1;
  GWEN_LIST_INIT(LCCL_CLIENT, cl);

  cl->openServices=GWEN_IdList_new();

  /* set default notify mask and flags */
  cl->notifyMask=~LC_NOTIFY_FLAGS_PRIVILEGED;
  cl->notifyFlags=0;

  /* assign unique id */
  cl->clientId=id;

  return cl;
}



void LCCL_Client_free(LCCL_CLIENT *cl){
  if (cl) {
    assert(cl->usage);
    if (--(cl->usage)==0) {
      DBG_MEM_DEC("LCCL_CLIENT");
      GWEN_LIST_FINI(LCCL_CLIENT, cl);
      GWEN_IdList_free(cl->openServices);
      free(cl->userName);
      free(cl->appName);
      GWEN_FREE_OBJECT(cl);
    }
  }
}



void LCCL_Client_Attach(LCCL_CLIENT *cl) {
  assert(cl);
  assert(cl->usage);
  cl->usage++;
}



const char *LCCL_Client_GetUserName(const LCCL_CLIENT *cl){
  assert(cl);
  return cl->userName;
}



void LCCL_Client_SetUserName(LCCL_CLIENT *cl, const char *s){
  assert(cl);
  free(cl->userName);
  if (s)
    cl->userName=strdup(s);
  else
    cl->userName=0;
}



GWEN_TYPE_UINT32 LCCL_Client_GetClientId(const LCCL_CLIENT *cl){
  assert(cl);
  return cl->clientId;
}



int LCCL_Client_HasService(const LCCL_CLIENT *cl, GWEN_TYPE_UINT32 id){
  assert(cl);
  return GWEN_IdList_HasId(cl->openServices, id);
}



int LCCL_Client_AddService(LCCL_CLIENT *cl, GWEN_TYPE_UINT32 id){
  assert(cl);
  return GWEN_IdList_AddId(cl->openServices, id);
}



int LCCL_Client_DelService(LCCL_CLIENT *cl, GWEN_TYPE_UINT32 id){
  assert(cl);
  return GWEN_IdList_DelId(cl->openServices, id);
}



GWEN_TYPE_UINT32 LCCL_Client_GetWaitRequestCount(const LCCL_CLIENT *cl){
  assert(cl);
  return cl->waitRequestCount;
}



void LCCL_Client_AddWaitRequestCount(LCCL_CLIENT *cl){
  assert(cl);
  cl->waitRequestCount++;
}



void LCCL_Client_SubWaitRequestCount(LCCL_CLIENT *cl){
  assert(cl);
  if (cl->waitRequestCount)
    cl->waitRequestCount--;
}



void LCCL_Client_ResetRequestCount(LCCL_CLIENT *cl) {
  assert(cl);
  cl->waitRequestCount=0;
}



const char *LCCL_Client_GetApplicationName(const LCCL_CLIENT *cl){
  assert(cl);
  return cl->appName;
}



void LCCL_Client_SetApplicationName(LCCL_CLIENT *cl, const char *s){
  assert(cl);
  free(cl->appName);
  if (s)
    cl->appName=strdup(s);
  else
    cl->appName=0;
}



GWEN_TYPE_UINT32 LCCL_Client_GetNotifyFlags(const LCCL_CLIENT *cl){
  assert(cl);
  return cl->notifyFlags;
}



void LCCL_Client_SetNotifyFlags(LCCL_CLIENT *cl, GWEN_TYPE_UINT32 flags){
  assert(cl);
  cl->notifyFlags=flags;
}



void LCCL_Client_AddNotifyFlags(LCCL_CLIENT *cl, GWEN_TYPE_UINT32 flags){
  assert(cl);
  cl->notifyFlags|=flags;
}



void LCCL_Client_DelNotifyFlags(LCCL_CLIENT *cl, GWEN_TYPE_UINT32 flags){
  assert(cl);
  cl->notifyFlags&=~flags;
}



GWEN_TYPE_UINT32 LCCL_Client_GetNotifyMask(const LCCL_CLIENT *cl){
  assert(cl);
  return cl->notifyMask;
}



void LCCL_Client_SetNotifyMask(LCCL_CLIENT *cl, GWEN_TYPE_UINT32 mask){
  assert(cl);
  cl->notifyMask=mask;
}



void LCCL_Client_AddNotifyMask(LCCL_CLIENT *cl, GWEN_TYPE_UINT32 mask){
  assert(cl);
  cl->notifyMask|=mask;
}



void LCCL_Client_DelNotifyMask(LCCL_CLIENT *cl, GWEN_TYPE_UINT32 mask){
  assert(cl);
  cl->notifyMask&=~mask;
}



int LCCL_Client_GetMaxClientLockTime(const LCCL_CLIENT *cl) {
  assert(cl);
  return cl->maxClientLockTime;
}



void LCCL_Client_SetMaxClientLockTime(LCCL_CLIENT *cl, int i) {
  assert(cl);
  cl->maxClientLockTime=i;
}



int LCCL_Client_GetMaxClientLocks(const LCCL_CLIENT *cl) {
  assert(cl);
  return cl->maxClientLocks;
}



void LCCL_Client_SetMaxClientLocks(LCCL_CLIENT *cl, int i) {
  assert(cl);
  cl->maxClientLocks=i;
}














