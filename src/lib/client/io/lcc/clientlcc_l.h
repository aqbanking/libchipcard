/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: client_p.h 137 2005-11-03 13:07:50Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CLIENT_CLIENTLCC_L_H
#define CHIPCARD_CLIENT_CLIENTLCC_L_H

#include "clientlcc.h"
#define LC_CARD_EXTEND_CLIENT
#include <chipcard/client/client_imp.h>



LC_CLIENTLCC_HANDLE_REQUEST_FN
LC_ClientLcc_SetHandleRequestFn(LC_CLIENT *cl,
                                LC_CLIENTLCC_HANDLE_REQUEST_FN fn);

int LC_ClientLcc_DeleteInRequest(LC_CLIENT *cl, GWEN_TYPE_UINT32 rid);

int LC_ClientLcc_SendResponse(LC_CLIENT *cl,
                              GWEN_TYPE_UINT32 rid,
                              GWEN_DB_NODE *rsp);

GWEN_TYPE_UINT32 LC_ClientLcc_SendRequest(LC_CLIENT *cl,
                                          LC_CARD *card,
                                          GWEN_TYPE_UINT32 serverId,
                                          GWEN_DB_NODE *dbReq);

GWEN_DB_NODE *LC_ClientLcc_GetNextResponse(LC_CLIENT *cl,
                                           GWEN_TYPE_UINT32 rqid);

GWEN_DB_NODE *LC_ClientLcc_WaitForNextResponse(LC_CLIENT *cl,
                                               GWEN_TYPE_UINT32 rqid,
                                               int timeout);

LC_CLIENT_RESULT
LC_ClientLcc_CheckResponse(LC_CLIENT *cl, GWEN_TYPE_UINT32 rid);

LC_CLIENT_RESULT LC_ClientLcc_CheckForError(GWEN_DB_NODE *db);


#endif /* CHIPCARD_CLIENT_CLIENTLCC_L_H */

