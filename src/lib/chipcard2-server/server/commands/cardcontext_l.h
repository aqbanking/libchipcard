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


#ifndef CHIPCARD_SERVER_CARDCONTEXT_L_H
#define CHIPCARD_SERVER_CARDCONTEXT_L_H

#include <gwenhywfar/xml.h>

typedef struct LC_CARDCONTEXT LC_CARDCONTEXT;

#include "cardmgr_l.h"


GWEN_LIST_FUNCTION_DEFS(LC_CARDCONTEXT, LC_CardContext)
GWEN_INHERIT_FUNCTION_DEFS(LC_CARDCONTEXT)

typedef LC_CARDMGR_RESULT
  (*LC_CARDCONTEXT_BUILDCMD)(LC_CARDCONTEXT *ctx,
                             LC_CARD *card,
                             GWEN_TYPE_UINT32 rid,
                             GWEN_DB_NODE *dbReq,
                             GWEN_DB_NODE *dbRsp);

typedef LC_CARDMGR_RESULT
  (*LC_CARDCONTEXT_CHECKCMD)(LC_CARDCONTEXT *ctx,
                             LC_REQUEST *rq,
                             GWEN_DB_NODE *dbDriverRsp,
                             GWEN_DB_NODE *dbRsp);


#include "cardmgr_l.h"


LC_CARDCONTEXT *LC_CardContext_new(LC_CARDMGR *mgr);
void LC_CardContext_free(LC_CARDCONTEXT *ctx);

void LC_CardContext_SetBuildCmdFn(LC_CARDCONTEXT *ctx,
                                  LC_CARDCONTEXT_BUILDCMD fn);

void LC_CardContext_SetCheckCmdFn(LC_CARDCONTEXT *ctx,
                                  LC_CARDCONTEXT_CHECKCMD fn);



LC_CARDMGR *LC_CardContext_GetManager(const LC_CARDCONTEXT *ctx);


LC_CARDMGR_RESULT LC_CardContext_BuildCmd(LC_CARDCONTEXT *ctx,
                                          LC_CARD *card,
                                          GWEN_TYPE_UINT32 rid,
                                          GWEN_DB_NODE *dbReq,
                                          GWEN_DB_NODE *dbRsp);

LC_CARDMGR_RESULT LC_CardContext_CheckCmd(LC_CARDCONTEXT *ctx,
                                          LC_REQUEST *rq,
                                          GWEN_DB_NODE *dbDriverRsp,
                                          GWEN_DB_NODE *dbRsp);


int LC_CardContext_CreateCommand(LC_CARDCONTEXT *ctx,
                                 const char *commandName,
                                 const char *driverType,
                                 const char *readerType,
                                 GWEN_BUFFER *gbuf,
                                 GWEN_DB_NODE *cmdData);


int LC_CardContext_ParseAnswer(LC_CARDCONTEXT *ctx,
                               GWEN_BUFFER *gbuf,
                               GWEN_DB_NODE *rspData);

GWEN_XMLNODE *LC_CardContext_GetCardNode(const LC_CARDCONTEXT *ctx);
void LC_CardContext_SetCardNode(LC_CARDCONTEXT *ctx,
                                GWEN_XMLNODE *n);
GWEN_XMLNODE *LC_CardContext_GetAppNode(const LC_CARDCONTEXT *ctx);
void LC_CardContext_SetAppNode(LC_CARDCONTEXT *ctx,
                               GWEN_XMLNODE *n);

GWEN_XMLNODE *LC_CardContext_GetDfNode(const LC_CARDCONTEXT *ctx);
void LC_CardContext_SetDfNode(LC_CARDCONTEXT *ctx,
                              GWEN_XMLNODE *n);
GWEN_XMLNODE *LC_CardContext_GetEfNode(const LC_CARDCONTEXT *ctx);
void LC_CardContext_SetEfNode(LC_CARDCONTEXT *ctx,
                              GWEN_XMLNODE *n);


GWEN_DB_NODE*
  LC_CardContext_TakeImmediateResponse(LC_CARDCONTEXT *ctx);

void LC_CardContext_SetImmediateResponse(LC_CARDCONTEXT *ctx,
                                         GWEN_DB_NODE *db);



#endif /* CHIPCARD_SERVER_CARDCONTEXT_L_H */


