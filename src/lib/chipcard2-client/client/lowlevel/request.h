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


#ifndef CHIPCARD_CLIENT_REQUEST_H
#define CHIPCARD_CLIENT_REQUEST_H

typedef struct LC_REQUEST LC_REQUEST;

#include <gwenhywfar/misc.h>
#include <gwenhywfar/db.h>

#include <time.h>

#include <chipcard2-client/client/card.h>


GWEN_LIST_FUNCTION_DEFS(LC_REQUEST, LC_Request);

LC_REQUEST *LC_Request_new(LC_CARD *card,
                           GWEN_DB_NODE *dbReq,
                           GWEN_TYPE_UINT32 serverId,
                           GWEN_TYPE_UINT32 ipcRequestId);
void LC_Request_free(LC_REQUEST *rq);

GWEN_DB_NODE *LC_Request_GetRequestData(const LC_REQUEST *rq);
time_t LC_Request_GetRequestTime(const LC_REQUEST *rq);

GWEN_TYPE_UINT32 LC_Request_GetRequestId(const LC_REQUEST *rq);
void LC_Request_SetRequestId(LC_REQUEST *rq, GWEN_TYPE_UINT32 rqid);

GWEN_TYPE_UINT32 LC_Request_GetIpcRequestId(const LC_REQUEST *rq);
void LC_Request_SetIpcRequestId(LC_REQUEST *rq, GWEN_TYPE_UINT32 rqid);

GWEN_TYPE_UINT32 LC_Request_GetServerId(const LC_REQUEST *rq);

LC_CARD *LC_Request_GetCard(const LC_REQUEST *rq);


int LC_Request_GetIsAborted(const LC_REQUEST *rq);
void LC_Request_SetIsAborted(LC_REQUEST *rq, int b);


#endif /* CHIPCARD_CLIENT_REQUEST_H */



