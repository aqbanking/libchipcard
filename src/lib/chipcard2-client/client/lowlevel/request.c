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


#include "request_p.h"
#include <gwenhywfar/debug.h>
#include <chipcard2/chipcard2.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>


GWEN_LIST_FUNCTIONS(LC_REQUEST, LC_Request);

static GWEN_TYPE_UINT32 LC_Request__lastRequestId=0;


LC_REQUEST *LC_Request_new(LC_CARD *card,
                           GWEN_DB_NODE *dbReq,
                           GWEN_TYPE_UINT32 serverId,
                           GWEN_TYPE_UINT32 ipcRequestId){
  LC_REQUEST *rq;

  assert(dbReq);

  GWEN_NEW_OBJECT(LC_REQUEST, rq);
  GWEN_LIST_INIT(LC_REQUEST, rq);
  rq->card=card;
  rq->requestData=dbReq;
  rq->requestTime=time(0);
  rq->ipcRequestId=ipcRequestId;
  rq->serverId=serverId;

  if (LC_Request__lastRequestId==0)
    LC_Request__lastRequestId=time(0);
  rq->requestId=++LC_Request__lastRequestId;
  return rq;
}



void LC_Request_free(LC_REQUEST *rq){
  if (rq) {
    GWEN_LIST_FINI(LC_REQUEST, rq);
    GWEN_DB_Group_free(rq->requestData);
    GWEN_FREE_OBJECT(rq);
  }
}



time_t LC_Request_GetRequestTime(const LC_REQUEST *rq){
  assert(rq);
  return rq->requestTime;
}



GWEN_TYPE_UINT32 LC_Request_GetRequestId(const LC_REQUEST *rq){
  assert(rq);
  return rq->requestId;
}



GWEN_TYPE_UINT32 LC_Request_GetServerId(const LC_REQUEST *rq){
  assert(rq);
  return rq->serverId;
}



GWEN_DB_NODE *LC_Request_GetRequestData(const LC_REQUEST *rq){
  assert(rq);
  return rq->requestData;
}



GWEN_TYPE_UINT32 LC_Request_GetIpcRequestId(const LC_REQUEST *rq){
  assert(rq);
  return rq->ipcRequestId;
}



void LC_Request_SetIpcRequestId(LC_REQUEST *rq, GWEN_TYPE_UINT32 rqid){
  assert(rq);
  rq->ipcRequestId=rqid;
}



void LC_Request_SetRequestId(LC_REQUEST *rq, GWEN_TYPE_UINT32 rqid){
  assert(rq);
  rq->requestId=rqid;
}



LC_CARD *LC_Request_GetCard(const LC_REQUEST *rq){
  assert(rq);
  return rq->card;
}



int LC_Request_GetIsAborted(const LC_REQUEST *rq){
  assert(rq);
  return rq->aborted;
}



void LC_Request_SetIsAborted(LC_REQUEST *rq, int b){
  assert(rq);
  rq->aborted=1;
}










