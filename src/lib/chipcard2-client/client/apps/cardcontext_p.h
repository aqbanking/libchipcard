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


#ifndef CHIPCARD_CLIENT_CARDCONTEXT_P_H
#define CHIPCARD_CLIENT_CARDCONTEXT_P_H

#include "cardcontext_l.h"


struct LC_CARDCONTEXT {
  GWEN_LIST_ELEMENT(LC_CARDCONTEXT);
  GWEN_INHERIT_ELEMENT(LC_CARDCONTEXT);
  LC_CARDMGR *mgr;
  GWEN_XMLNODE *appNode;
  GWEN_XMLNODE *dfNode;
  GWEN_XMLNODE *efNode;
  GWEN_XMLNODE *cmdNode;
  GWEN_XMLNODE *tmpFileNode;
};


GWEN_XMLNODE *LC_CardContext_FindFile(LC_CARDCONTEXT *ctx,
                                      const char *type,
                                      const char *name);


LC_CARDMGR_RESULT LC_CardContext_HandleSelectDF(LC_CARDCONTEXT *ctx,
                                                GWEN_DB_NODE *dbReq);


LC_CARDMGR_RESULT LC_CardContext_HandleSelectEF(LC_CARDCONTEXT *ctx,
                                                GWEN_DB_NODE *dbReq);
LC_CARDMGR_RESULT LC_CardContext_HandleReadRecord(LC_CARDCONTEXT *ctx,
                                                  GWEN_DB_NODE *dbReq);


#endif /* CHIPCARD_CLIENT_CARDCONTEXT_P_H */


