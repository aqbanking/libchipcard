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
#include <gwenhywfar/misc.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>



GWEN_LIST_FUNCTIONS(LC_REQUEST, LC_Request);


static GWEN_TYPE_UINT32 LC_Request_LastId=0;


LC_REQUEST *LC_Request_new(LC_CLIENT *cl,
                           GWEN_DB_NODE *inRequestData,
                           GWEN_DB_NODE *outRequestData,
                           LC_CARD *card){
  LC_REQUEST *rq;

  assert(inRequestData);
  GWEN_NEW_OBJECT(LC_REQUEST, rq);
  DBG_MEM_INC("LC_REQUEST", 0);
  GWEN_LIST_INIT(LC_REQUEST, rq);
  rq->client=cl;
  rq->inRequestData=inRequestData;
  rq->outRequestData=outRequestData;
  rq->card=card;

  /* assign unique id */
  if (LC_Request_LastId==0)
    LC_Request_LastId=time(0);
  rq->requestId=++LC_Request_LastId;
  rq->usage=1;
  return rq;
}



void LC_Request_free(LC_REQUEST *rq){
  if (rq) {
    DBG_MEM_DEC("LC_REQUEST");
    assert(rq->usage);
    if (--(rq->usage)==0) {
      GWEN_LIST_FINI(LC_REQUEST, rq);

      GWEN_DB_Group_free(rq->outRequestData);
      GWEN_DB_Group_free(rq->inRequestData);

      GWEN_FREE_OBJECT(rq);
    }
  }
}



LC_CARD *LC_Request_GetCard(const LC_REQUEST *rq){
  assert(rq);
  return rq->card;
}



GWEN_TYPE_UINT32 LC_Request_GetRequestId(const LC_REQUEST *rq){
  assert(rq);
  return rq->requestId;
}



LC_CLIENT *LC_Request_GetClient(const LC_REQUEST *rq){
  assert(rq);
  return rq->client;
}



GWEN_TYPE_UINT32 LC_Request_GetInRequestId(const LC_REQUEST *rq){
  assert(rq);
  return rq->inRequestId;
}



GWEN_TYPE_UINT32 LC_Request_GetOutRequestId(const LC_REQUEST *rq){
  assert(rq);
  return rq->outRequestId;
}



void LC_Request_SetInRequestId(LC_REQUEST *rq,
                               GWEN_TYPE_UINT32 id){
  assert(rq);
  rq->inRequestId=id;
}



GWEN_DB_NODE *LC_Request_GetInRequestData(const LC_REQUEST *rq){
  assert(rq);
  return rq->inRequestData;
}



void LC_Request_SetOutRequestId(LC_REQUEST *rq,
                                GWEN_TYPE_UINT32 id){
  assert(rq);
  rq->outRequestId=id;
}



GWEN_DB_NODE *LC_Request_GetOutRequestData(const LC_REQUEST *rq){
  assert(rq);
  return rq->outRequestData;
}



void LC_Request_SetOutRequestData(LC_REQUEST *rq, GWEN_DB_NODE *db){

  assert(rq);
  assert(db);
  GWEN_DB_Group_free(rq->outRequestData);
  rq->outRequestData=db;
}

















