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


#ifndef CHIPCARD_SERVER_CARDCONTEXT_P_H
#define CHIPCARD_SERVER_CARDCONTEXT_P_H

#include "cardcontext_l.h"


struct LC_CARDCONTEXT {
  GWEN_LIST_ELEMENT(LC_CARDCONTEXT);
  GWEN_INHERIT_ELEMENT(LC_CARDCONTEXT);
  LC_CARDCONTEXT_BUILDCMD buildCmdFn;
  LC_CARDCONTEXT_CHECKCMD checkCmdFn;
  LC_CARDMGR *mgr;
  GWEN_XMLNODE *cardNode;
  GWEN_XMLNODE *cmdNode;
  GWEN_TYPE_UINT32 usage;
};


GWEN_XMLNODE *LC_CardContext__FindResult(LC_CARDCONTEXT *ctx,
                                         GWEN_XMLNODE *cmd,
                                         int sw1,
                                         int sw2);


GWEN_XMLNODE *LC_CardContext_FindResult(LC_CARDCONTEXT *ctx,
                                        GWEN_XMLNODE *cmd,
                                        int sw1,
                                        int sw2);
int LC_CardContext_ParseResult(LC_CARDCONTEXT *ctx,
                               GWEN_XMLNODE *node,
                               GWEN_BUFFER *gbuf,
                               GWEN_DB_NODE *rspData);

GWEN_XMLNODE *LC_CardContext_FindResponse(LC_CARDCONTEXT *ctx,
                                          GWEN_XMLNODE *cmd,
                                          const char *typ);

int LC_CardContext_ParseResponse(LC_CARDCONTEXT *ctx,
                                 GWEN_XMLNODE *node,
                                 GWEN_BUFFER *gbuf,
                                 GWEN_DB_NODE *rspData);


GWEN_XMLNODE *LC_CardContext__FindCommand(GWEN_XMLNODE *node,
                                          const char *commandName,
                                          const char *driverType,
                                          const char *readerType);

GWEN_XMLNODE *LC_CardContext_FindCommand(LC_CARDCONTEXT *ctx,
                                         const char *commandName,
                                         const char *driverType,
                                         const char *readerType);

int LC_CardContext_CreateAPDU(LC_CARDCONTEXT *ctx,
                              GWEN_XMLNODE *node,
                              GWEN_BUFFER *gbuf,
                              GWEN_DB_NODE *cmdData);

int LC_CardContext_CreateGenericCommand(LC_CARDCONTEXT *ctx,
                                        LC_CARD *card,
                                        GWEN_TYPE_UINT32 rid,
                                        const char *cmd,
                                        GWEN_DB_NODE *dbCmd);


LC_CARDMGR_RESULT LC_CardContext__BuildCmd(LC_CARDCONTEXT *ctx,
                                           LC_CARD *card,
                                           GWEN_TYPE_UINT32 rid,
                                           GWEN_DB_NODE *dbReq,
                                           GWEN_DB_NODE *dbRsp);

LC_CARDMGR_RESULT LC_CardContext__CheckCmd(LC_CARDCONTEXT *ctx,
                                           LC_REQUEST *rq,
                                           GWEN_DB_NODE *dbDriverRsp,
                                           GWEN_DB_NODE *dbRsp);

GWEN_XMLNODE *LC_CardContext__FindFlags(GWEN_XMLNODE *node,
                                        const char *driverType,
                                        const char *readerType);
GWEN_XMLNODE *LC_CardContext_FindFlags(LC_CARDCONTEXT *ctx,
                                       const char *driverType,
                                       const char *readerType);

GWEN_TYPE_UINT32 LC_CardContext_GetReaderFlags(LC_CARD *card);


#endif /* CHIPCARD_SERVER_CARDCONTEXT_P_H */


