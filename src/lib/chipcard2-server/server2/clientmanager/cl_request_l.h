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



#ifndef CHIPCARD_SERVER2_CL_REQUEST_L_H
#define CHIPCARD_SERVER2_CL_REQUEST_L_H



#include <gwenhywfar/request.h>
#include "clientmanager_l.h"
#include "cl_client_l.h"
#include "common/card.h"


typedef struct LCCL_REQUEST LCCL_REQUEST;


GWEN_IPC_REQUEST *LCCL_Request_new();

LCCO_CARD *LCCL_Request_GetCard(const GWEN_IPC_REQUEST *rq);
void LCCL_Request_SetCard(GWEN_IPC_REQUEST *rq, LCCO_CARD *card);

LCCL_CLIENTMANAGER *LCCL_Request_GetClientManager(const GWEN_IPC_REQUEST *rq);
void LCCL_Request_SetClientManager(GWEN_IPC_REQUEST *rq,
                                   LCCL_CLIENTMANAGER *clm);

LCCL_CLIENT *LCCL_Request_GetClient(const GWEN_IPC_REQUEST *rq);
void LCCL_Request_SetClient(GWEN_IPC_REQUEST *rq, LCCL_CLIENT *cl);

GWEN_TYPE_UINT32 LCCL_Request_GetUint32Data(const GWEN_IPC_REQUEST *rq);
void LCCL_Request_SetUint32Data(GWEN_IPC_REQUEST *rq, GWEN_TYPE_UINT32 i);

GWEN_TYPE_UINT32 LCCL_Request_GetUint32Data2(const GWEN_IPC_REQUEST *rq);
void LCCL_Request_SetUint32Data2(GWEN_IPC_REQUEST *rq, GWEN_TYPE_UINT32 i);



#endif /* CHIPCARD_SERVER2_CL_REQUEST_L_H */


