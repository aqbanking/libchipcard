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


#ifndef CHIPCARD_CLIENT_CARDCONTEXT_L_H
#define CHIPCARD_CLIENT_CARDCONTEXT_L_H

typedef struct LC_CARDCONTEXT LC_CARDCONTEXT;

#include "cardmgr_l.h"

#include <gwenhywfar/misc.h>
#include <gwenhywfar/xml.h>
#include <gwenhywfar/db.h>
#include <gwenhywfar/buffer.h>
#include <gwenhywfar/inherit.h>

GWEN_LIST_FUNCTION_DEFS(LC_CARDCONTEXT, LC_CardContext)
GWEN_INHERIT_FUNCTION_DEFS(LC_CARDCONTEXT)


LC_CARDCONTEXT *LC_CardContext_new(LC_CARDMGR *mgr);
void LC_CardContext_free(LC_CARDCONTEXT *ctx);


LC_CARDMGR *LC_CardContext_GetManager(const LC_CARDCONTEXT *ctx);


LC_CARDMGR_RESULT LC_CardContext_Translate(LC_CARDCONTEXT *ctx,
                                           GWEN_DB_NODE *dbReq);

LC_CARDMGR_RESULT LC_CardContext_CheckResponse(LC_CARDCONTEXT *ctx,
                                               GWEN_DB_NODE *dbReq,
                                               GWEN_DB_NODE *dbServerRsp);


GWEN_XMLNODE *LC_CardContext_GetAppNode(const LC_CARDCONTEXT *ctx);
void LC_CardContext_SetAppNode(LC_CARDCONTEXT *ctx,
                               GWEN_XMLNODE *n);

GWEN_XMLNODE *LC_CardContext_GetDfNode(const LC_CARDCONTEXT *ctx);
void LC_CardContext_SetDfNode(LC_CARDCONTEXT *ctx,
                              GWEN_XMLNODE *n);
GWEN_XMLNODE *LC_CardContext_GetEfNode(const LC_CARDCONTEXT *ctx);
void LC_CardContext_SetEfNode(LC_CARDCONTEXT *ctx,
                              GWEN_XMLNODE *n);

int LC_CardContext_GetRecordNumber(LC_CARDCONTEXT *ctx,
                                   const char *recName);

int LC_CardContext_ParseRecord(LC_CARDCONTEXT *ctx,
                               int recNum,
                               GWEN_BUFFER *buf,
                               GWEN_DB_NODE *dbRecord);
int LC_CardContext_CreateRecord(LC_CARDCONTEXT *ctx,
                                int recNum,
                                GWEN_BUFFER *buf,
                                GWEN_DB_NODE *dbRecord);

int LC_CardContext_ParseData(LC_CARDCONTEXT *ctx,
                             const char *format,
                             GWEN_BUFFER *buf,
                             GWEN_DB_NODE *dbData);
int LC_CardContext_CreateData(LC_CARDCONTEXT *ctx,
                              const char *format,
                              GWEN_BUFFER *buf,
                              GWEN_DB_NODE *dbData);




#endif /* CHIPCARD_CLIENT_CARDCONTEXT_L_H */



