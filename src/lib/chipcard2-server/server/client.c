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


#include "client_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LC_CLIENT, LC_Client);


LC_CLIENT *LC_Client_new(GWEN_TYPE_UINT32 id){
  LC_CLIENT *cl;

  GWEN_NEW_OBJECT(LC_CLIENT, cl);
  GWEN_LIST_INIT(LC_CLIENT, cl);

  cl->seenCards=GWEN_IdList_new();
  cl->selectedReaders=GWEN_IdList_new();
  cl->openServices=GWEN_IdList_new();

  /* set default notify mask and flags */
  cl->notifyMask=~LC_NOTIFY_FLAGS_PRIVILEGED;
  cl->notifyFlags=0;

  /* assign unique id */
  cl->clientId=id;

  return cl;
}



void LC_Client_free(LC_CLIENT *cl){
  if (cl) {
    GWEN_LIST_FINI(LC_CLIENT, cl);
    GWEN_IdList_free(cl->seenCards);
    GWEN_IdList_free(cl->selectedReaders);
    GWEN_IdList_free(cl->openServices);
    free(cl->userName);
    GWEN_FREE_OBJECT(cl);
  }
}



const char *LC_Client_GetUserName(const LC_CLIENT *cl){
  assert(cl);
  return cl->userName;
}



void LC_Client_SetUserName(LC_CLIENT *cl, const char *s){
  assert(cl);
  free(cl->userName);
  if (s)
    cl->userName=strdup(s);
  else
    cl->userName=0;
}



GWEN_TYPE_UINT32 LC_Client_GetClientId(const LC_CLIENT *cl){
  assert(cl);
  return cl->clientId;
}



int LC_Client_HasReader(const LC_CLIENT *cl, GWEN_TYPE_UINT32 id){
  assert(cl);
  return GWEN_IdList_HasId(cl->selectedReaders, id);
}



int LC_Client_AddReader(const LC_CLIENT *cl, GWEN_TYPE_UINT32 id){
  assert(cl);
  return GWEN_IdList_AddId(cl->selectedReaders, id);
}



int LC_Client_DelReader(const LC_CLIENT *cl, GWEN_TYPE_UINT32 id){
  assert(cl);
  return GWEN_IdList_DelId(cl->selectedReaders, id);
}



int LC_Client_HasCard(const LC_CLIENT *cl, GWEN_TYPE_UINT32 id){
  assert(cl);
  return GWEN_IdList_HasId(cl->seenCards, id);
}



int LC_Client_AddCard(LC_CLIENT *cl, GWEN_TYPE_UINT32 id){
  assert(cl);
  return GWEN_IdList_AddId(cl->seenCards, id);
}



int LC_Client_DelCard(LC_CLIENT *cl, GWEN_TYPE_UINT32 id){
  assert(cl);
  return GWEN_IdList_DelId(cl->seenCards, id);
}



void LC_Client_DelAllCards(LC_CLIENT *cl){
  GWEN_IdList_Clear(cl->seenCards);
}



int LC_Client_HasService(const LC_CLIENT *cl, GWEN_TYPE_UINT32 id){
  assert(cl);
  return GWEN_IdList_HasId(cl->openServices, id);
}



int LC_Client_AddService(LC_CLIENT *cl, GWEN_TYPE_UINT32 id){
  assert(cl);
  return GWEN_IdList_AddId(cl->openServices, id);
}



int LC_Client_DelService(LC_CLIENT *cl, GWEN_TYPE_UINT32 id){
  assert(cl);
  return GWEN_IdList_DelId(cl->openServices, id);
}



GWEN_TYPE_UINT32 LC_Client_GetWaitRequestCount(const LC_CLIENT *cl){
  assert(cl);
  return cl->waitRequestCount;
}



void LC_Client_AddWaitRequestCount(LC_CLIENT *cl){
  assert(cl);
  cl->waitRequestCount++;
}



void LC_Client_SubWaitRequestCount(LC_CLIENT *cl){
  assert(cl);
  if (cl->waitRequestCount)
    cl->waitRequestCount--;
  if (cl->waitRequestCount==0) {
    cl->waitReaderFlags=0;
    cl->waitReaderMask=0;
  }
}



const char *LC_Client_GetApplicationName(const LC_CLIENT *cl){
  assert(cl);
  return cl->appName;
}



void LC_Client_SetApplicationName(LC_CLIENT *cl, const char *s){
  assert(cl);
  free(cl->appName);
  if (s)
    cl->appName=strdup(s);
  else
    cl->appName=0;
}



GWEN_TYPE_UINT32 LC_Client_GetWaitReaderFlags(const LC_CLIENT *cl){
  assert(cl);
  return cl->waitReaderFlags;
}



GWEN_TYPE_UINT32 LC_Client_GetWaitReaderMask(const LC_CLIENT *cl){
  assert(cl);
  return cl->waitReaderMask;
}



void LC_Client_AddWaitReaderState(LC_CLIENT *cl,
                                  GWEN_TYPE_UINT32 flags,
                                  GWEN_TYPE_UINT32 mask){
  assert(cl);
  cl->waitReaderFlags|=flags;
  cl->waitReaderMask|=mask;
}



void LC_Client_ResetWaitReaderState(LC_CLIENT *cl) {
  assert(cl);
  cl->waitReaderFlags=0;
  cl->waitReaderMask=0;
}



GWEN_TYPE_UINT32 LC_Client_GetNotifyFlags(const LC_CLIENT *cl){
  assert(cl);
  return cl->notifyFlags;
}



void LC_Client_SetNotifyFlags(LC_CLIENT *cl,
			      GWEN_TYPE_UINT32 flags){
  assert(cl);
  cl->notifyFlags=flags;
}



void LC_Client_AddNotifyFlags(LC_CLIENT *cl,
			      GWEN_TYPE_UINT32 flags){
  assert(cl);
  cl->notifyFlags|=flags;
}



void LC_Client_DelNotifyFlags(LC_CLIENT *cl,
			      GWEN_TYPE_UINT32 flags){
  assert(cl);
  cl->notifyFlags&=~flags;
}



GWEN_TYPE_UINT32 LC_Client_GetNotifyMask(const LC_CLIENT *cl){
  assert(cl);
  return cl->notifyMask;
}



void LC_Client_SetNotifyMask(LC_CLIENT *cl,
			      GWEN_TYPE_UINT32 mask){
  assert(cl);
  cl->notifyMask=mask;
}



void LC_Client_AddNotifyMask(LC_CLIENT *cl,
			      GWEN_TYPE_UINT32 mask){
  assert(cl);
  cl->notifyMask|=mask;
}



void LC_Client_DelNotifyMask(LC_CLIENT *cl,
			      GWEN_TYPE_UINT32 mask){
  assert(cl);
  cl->notifyMask&=~mask;
}



GWEN_TYPE_UINT32 LC_Client_GetLastWaitRequestId(const LC_CLIENT *cl){
  assert(cl);
  return cl->lastWaitRequestId;
}



void LC_Client_SetLastWaitRequestId(LC_CLIENT *cl, GWEN_TYPE_UINT32 id){
  assert(cl);
  cl->lastWaitRequestId=id;
}














