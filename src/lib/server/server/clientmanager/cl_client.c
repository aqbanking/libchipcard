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
#include <chipcard/chipcard.h>

#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LCCL_CLIENT, LCCL_Client);


LCCL_CLIENT *LCCL_Client_new(uint32_t id){
  LCCL_CLIENT *cl;

  GWEN_NEW_OBJECT(LCCL_CLIENT, cl);
  DBG_MEM_INC("LCCL_CLIENT", 0);
  cl->usage=1;
  GWEN_LIST_INIT(LCCL_CLIENT, cl);

  cl->openServices=GWEN_IdList_new();
  cl->usedReaders=GWEN_IdList_new();

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
      GWEN_IdList_free(cl->usedReaders);
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



uint32_t LCCL_Client_GetClientId(const LCCL_CLIENT *cl){
  assert(cl);
  return cl->clientId;
}



int LCCL_Client_HasService(const LCCL_CLIENT *cl, uint32_t id){
  assert(cl);
  return GWEN_IdList_HasId(cl->openServices, id);
}



int LCCL_Client_AddService(LCCL_CLIENT *cl, uint32_t id){
  assert(cl);
  return GWEN_IdList_AddId(cl->openServices, id);
}



int LCCL_Client_DelService(LCCL_CLIENT *cl, uint32_t id){
  assert(cl);
  return GWEN_IdList_DelId(cl->openServices, id);
}



int LCCL_Client_AddReader(LCCL_CLIENT *cl, uint32_t id){
  assert(cl);
  return GWEN_IdList_AddId(cl->usedReaders, id);
}



int LCCL_Client_DelReader(LCCL_CLIENT *cl, uint32_t id){
  assert(cl);
  return GWEN_IdList_DelId(cl->usedReaders, id);
}



uint32_t LCCL_Client_GetFirstReader(LCCL_CLIENT *cl){
  assert(cl);
  return GWEN_IdList_GetFirstId(cl->usedReaders);
}



uint32_t LCCL_Client_GetNextReader(LCCL_CLIENT *cl){
  assert(cl);
  return GWEN_IdList_GetNextId(cl->usedReaders);
}



uint32_t LCCL_Client_GetWaitRequestCount(const LCCL_CLIENT *cl){
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



uint32_t LCCL_Client_GetNotifyFlags(const LCCL_CLIENT *cl){
  assert(cl);
  return cl->notifyFlags;
}



void LCCL_Client_SetNotifyFlags(LCCL_CLIENT *cl, uint32_t flags){
  assert(cl);
  cl->notifyFlags=flags;
}



void LCCL_Client_AddNotifyFlags(LCCL_CLIENT *cl, uint32_t flags){
  assert(cl);
  cl->notifyFlags|=flags;
}



void LCCL_Client_DelNotifyFlags(LCCL_CLIENT *cl, uint32_t flags){
  assert(cl);
  cl->notifyFlags&=~flags;
}



uint32_t LCCL_Client_GetNotifyMask(const LCCL_CLIENT *cl){
  assert(cl);
  return cl->notifyMask;
}



void LCCL_Client_SetNotifyMask(LCCL_CLIENT *cl, uint32_t mask){
  assert(cl);
  cl->notifyMask=mask;
}



void LCCL_Client_AddNotifyMask(LCCL_CLIENT *cl, uint32_t mask){
  assert(cl);
  cl->notifyMask|=mask;
}



void LCCL_Client_DelNotifyMask(LCCL_CLIENT *cl, uint32_t mask){
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



int LCCL_Client_GetWantDestroy(const LCCL_CLIENT *cl) {
  assert(cl);
  return cl->destroy;
}



void LCCL_Client_SetWantDestroy(LCCL_CLIENT *cl, int i) {
  assert(cl);
  cl->destroy=i;
}



void LCCL_Client_Dump(const LCCL_CLIENT *cl, FILE *f, int indent) {
  int i;
  GWEN_DB_NODE *dbT;

  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "--------------------------\n");
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Client\n");
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Id           : %08x\n", cl->clientId);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "App          : %s\n", cl->appName);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "User         : %s\n", cl->userName);

  dbT=GWEN_DB_Group_new("notify");
  LC_NotifyFlags_toDb(dbT, "flags", cl->notifyFlags);
  LC_NotifyFlags_toDb(dbT, "mask", cl->notifyFlags);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Notify flags : ");
  for (i=0; ; i++) {
    const char *s;

    s=GWEN_DB_GetCharValue(dbT, "flags", i, 0);
    if (!s)
      break;
    if (i)
      fprintf(f, ", ");
    fprintf(f, "%s", s);
  }
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Notify mask  : ");
  for (i=0; ; i++) {
    const char *s;

    s=GWEN_DB_GetCharValue(dbT, "mask", i, 0);
    if (!s)
      break;
    if (i)
      fprintf(f, ", ");
    fprintf(f, "%s", s);
  }
  GWEN_DB_Group_free(dbT);
  fprintf(stderr, "\n");
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Wait requests: %d\n", cl->waitRequestCount);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Want destroy : %d\n", cl->destroy);
}











