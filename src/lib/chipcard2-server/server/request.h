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


#ifndef CHIPCARD_SERVER_REQUEST_H
#define CHIPCARD_SERVER_REQUEST_H

typedef struct LC_REQUEST LC_REQUEST;

#include <gwenhywfar/misc.h>
#include <chipcard2-server/server/card.h>
#include <chipcard2-server/server/client.h>


GWEN_LIST_FUNCTION_DEFS(LC_REQUEST, LC_Request);


LC_REQUEST *LC_Request_new(LC_CLIENT *cl,
                           GWEN_DB_NODE *inRequestData,
                           GWEN_DB_NODE *outRequestData,
                           LC_CARD *card);
void LC_Request_free(LC_REQUEST *rq);

LC_CARD *LC_Request_GetCard(const LC_REQUEST *rq);
LC_CLIENT *LC_Request_GetClient(const LC_REQUEST *rq);

GWEN_TYPE_UINT32 LC_Request_GetInRequestId(const LC_REQUEST *rq);
void LC_Request_SetInRequestId(LC_REQUEST *rq,
                               GWEN_TYPE_UINT32 id);
GWEN_DB_NODE *LC_Request_GetInRequestData(const LC_REQUEST *rq);

GWEN_TYPE_UINT32 LC_Request_GetOutRequestId(const LC_REQUEST *rq);
void LC_Request_SetOutRequestId(LC_REQUEST *rq,
                                GWEN_TYPE_UINT32 id);
GWEN_DB_NODE *LC_Request_GetOutRequestData(const LC_REQUEST *rq);
/**
 * Takes over ownership of the given db node.
 */
void LC_Request_SetOutRequestData(LC_REQUEST *rq, GWEN_DB_NODE *db);

GWEN_TYPE_UINT32 LC_Request_GetRequestId(const LC_REQUEST *rq);


#endif /* CHIPCARD_SERVER_REQUEST_H */


