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


#ifndef CHIPCARD_SERVER_CARDMGR_L_H
#define CHIPCARD_SERVER_CARDMGR_L_H

#include <gwenhywfar/stringlist.h>
#include <gwenhywfar/db.h>
#include <gwenhywfar/buffer.h>
#include <gwenhywfar/msgengine.h>

typedef struct LC_CARDMGR LC_CARDMGR;

typedef enum {
  LC_CardMgr_ResultOk=0,
  LC_CardMgr_ResultNeedMore,
  LC_CardMgr_ResultImmediateResponse,
  LC_CardMgr_ResultError,
  LC_CardMgr_ResultCmdError
} LC_CARDMGR_RESULT;

#include <chipcard2-server/server/cardserver.h>
#include <chipcard2-server/server/request.h>




LC_CARDMGR *LC_CardMgr_new(const GWEN_STRINGLIST *paths);

void LC_CardMgr_free(LC_CARDMGR *mgr);
void LC_CardMgr_Attach(LC_CARDMGR *mgr);

GWEN_MSGENGINE *LC_CardMgr_GetMsgEngine(const LC_CARDMGR *mgr);

LC_CARDMGR_RESULT LC_CardMgr_CheckResponse(LC_CARDMGR *mgr,
                                           LC_REQUEST *rq,
                                           GWEN_DB_NODE *dbDriverRsp,
                                           GWEN_DB_NODE *dbRsp);

LC_CARDMGR_RESULT LC_CardMgr_HandleCommand(LC_CARDMGR *mgr,
                                           LC_CARD *card,
                                           GWEN_TYPE_UINT32 rid,
                                           GWEN_DB_NODE *dbReq,
                                           GWEN_DB_NODE *dbRsp);


int LC_CardMgr_SelectCard(LC_CARDMGR *mgr,
                          LC_CARD *card,
                          const char *cardName);

GWEN_XMLNODE *LC_CardMgr_FindCardNode(LC_CARDMGR *mgr,
                                      const char *cardName);

#endif /* CHIPCARD_SERVER_CARDMGR_L_H */


