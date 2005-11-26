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


#include "cl_request_p.h"
#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>


GWEN_INHERIT(GWEN_IPC_REQUEST, LCCL_REQUEST)


GWEN_IPC_REQUEST *LCCL_Request_new() {
  GWEN_IPC_REQUEST *rq;
  LCCL_REQUEST *srq;

  rq=GWEN_IpcRequest_new();
  GWEN_NEW_OBJECT(LCCL_REQUEST, srq);
  GWEN_INHERIT_SETDATA(GWEN_IPC_REQUEST, LCCL_REQUEST, rq, srq,
                       LCCL_Request_FreeData);

  return rq;
}



void LCCL_Request_FreeData(void *bp, void *p) {
  LCCL_REQUEST *srq;

  srq=(LCCL_REQUEST*)p;
  LCCL_Client_free(srq->client);
  LCCO_Card_free(srq->card);
  GWEN_FREE_OBJECT(srq);
}



LCCO_CARD *LCCL_Request_GetCard(const GWEN_IPC_REQUEST *rq) {
  LCCL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCCL_REQUEST, rq);
  assert(srq);

  return srq->card;
}



void LCCL_Request_SetCard(GWEN_IPC_REQUEST *rq, LCCO_CARD *card) {
  LCCL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCCL_REQUEST, rq);
  assert(srq);

  if (card)
    LCCO_Card_Attach(card);
  LCCO_Card_free(srq->card);
  srq->card=card;
}



LCCL_CLIENTMANAGER *LCCL_Request_GetClientManager(const GWEN_IPC_REQUEST *rq){
  LCCL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCCL_REQUEST, rq);
  assert(srq);

  return srq->clientManager;
}



void LCCL_Request_SetClientManager(GWEN_IPC_REQUEST *rq,
                                   LCCL_CLIENTMANAGER *clm) {
  LCCL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCCL_REQUEST, rq);
  assert(srq);

  srq->clientManager=clm;
}



LCCL_CLIENT *LCCL_Request_GetClient(const GWEN_IPC_REQUEST *rq) {
  LCCL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCCL_REQUEST, rq);
  assert(srq);

  return srq->client;
}



void LCCL_Request_SetClient(GWEN_IPC_REQUEST *rq, LCCL_CLIENT *cl) {
  LCCL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCCL_REQUEST, rq);
  assert(srq);

  if (cl)
    LCCL_Client_Attach(cl);
  LCCL_Client_free(srq->client);
  srq->client=cl;
}



GWEN_TYPE_UINT32 LCCL_Request_GetUint32Data(const GWEN_IPC_REQUEST *rq) {
  LCCL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCCL_REQUEST, rq);
  assert(srq);

  return srq->uint32Data1;
}



void LCCL_Request_SetUint32Data(GWEN_IPC_REQUEST *rq, GWEN_TYPE_UINT32 i) {
  LCCL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCCL_REQUEST, rq);
  assert(srq);

  srq->uint32Data1=i;
}



GWEN_TYPE_UINT32 LCCL_Request_GetUint32Data2(const GWEN_IPC_REQUEST *rq) {
  LCCL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCCL_REQUEST, rq);
  assert(srq);

  return srq->uint32Data2;
}



void LCCL_Request_SetUint32Data2(GWEN_IPC_REQUEST *rq, GWEN_TYPE_UINT32 i) {
  LCCL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCCL_REQUEST, rq);
  assert(srq);

  srq->uint32Data2=i;
}











