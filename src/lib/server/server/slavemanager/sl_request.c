/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: cl_request.c 211 2006-09-07 23:57:04Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "sl_request_p.h"
#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>


GWEN_INHERIT(GWEN_IPC_REQUEST, LCSL_REQUEST)


GWEN_IPC_REQUEST *LCSL_Request_new() {
  GWEN_IPC_REQUEST *rq;
  LCSL_REQUEST *srq;

  rq=GWEN_IpcRequest_new();
  GWEN_NEW_OBJECT(LCSL_REQUEST, srq);
  GWEN_INHERIT_SETDATA(GWEN_IPC_REQUEST, LCSL_REQUEST, rq, srq,
                       LCSL_Request_FreeData);

  return rq;
}



void GWENHYWFAR_CB LCSL_Request_FreeData(void *bp, void *p) {
  LCSL_REQUEST *srq;

  srq=(LCSL_REQUEST*)p;
  LCCO_Card_free(srq->card);
  LCCO_Reader_free(srq->reader);
  GWEN_FREE_OBJECT(srq);
}



LCCO_READER *LCSL_Request_GetReader(const GWEN_IPC_REQUEST *rq) {
  LCSL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCSL_REQUEST, rq);
  assert(srq);

  return srq->reader;
}



void LCSL_Request_SetReader(GWEN_IPC_REQUEST *rq, LCCO_READER *r) {
  LCSL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCSL_REQUEST, rq);
  assert(srq);

  if (r)
    LCCO_Reader_Attach(r);
  LCCO_Reader_free(srq->reader);
  srq->reader=r;
}



LCCO_CARD *LCSL_Request_GetCard(const GWEN_IPC_REQUEST *rq) {
  LCSL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCSL_REQUEST, rq);
  assert(srq);

  return srq->card;
}



void LCSL_Request_SetCard(GWEN_IPC_REQUEST *rq, LCCO_CARD *card) {
  LCSL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCSL_REQUEST, rq);
  assert(srq);

  if (card)
    LCCO_Card_Attach(card);
  LCCO_Card_free(srq->card);
  srq->card=card;
}



LCSL_SLAVEMANAGER *LCSL_Request_GetSlaveManager(const GWEN_IPC_REQUEST *rq){
  LCSL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCSL_REQUEST, rq);
  assert(srq);

  return srq->slaveManager;
}



void LCSL_Request_SetSlaveManager(GWEN_IPC_REQUEST *rq,
                                  LCSL_SLAVEMANAGER *slm) {
  LCSL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCSL_REQUEST, rq);
  assert(srq);

  srq->slaveManager=slm;
}



GWEN_TYPE_UINT32 LCSL_Request_GetUint32Data(const GWEN_IPC_REQUEST *rq) {
  LCSL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCSL_REQUEST, rq);
  assert(srq);

  return srq->uint32Data1;
}



void LCSL_Request_SetUint32Data(GWEN_IPC_REQUEST *rq, GWEN_TYPE_UINT32 i) {
  LCSL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCSL_REQUEST, rq);
  assert(srq);

  srq->uint32Data1=i;
}



GWEN_TYPE_UINT32 LCSL_Request_GetUint32Data2(const GWEN_IPC_REQUEST *rq) {
  LCSL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCSL_REQUEST, rq);
  assert(srq);

  return srq->uint32Data2;
}



void LCSL_Request_SetUint32Data2(GWEN_IPC_REQUEST *rq, GWEN_TYPE_UINT32 i) {
  LCSL_REQUEST *srq;

  assert(rq);
  srq=GWEN_INHERIT_GETDATA(GWEN_IPC_REQUEST, LCSL_REQUEST, rq);
  assert(srq);

  srq->uint32Data2=i;
}











