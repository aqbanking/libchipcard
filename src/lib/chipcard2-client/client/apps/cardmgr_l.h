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


#ifndef CHIPCARD_CLIENT_CARDMGR_L_H
#define CHIPCARD_CLIENT_CARDMGR_L_H

#include <gwenhywfar/stringlist.h>
#include <gwenhywfar/db.h>
#include <gwenhywfar/buffer.h>
#include <gwenhywfar/msgengine.h>

typedef struct LC_CARDMGR LC_CARDMGR;

typedef enum {
  LC_CardMgr_ResultOk=0,
  LC_CardMgr_ResultError,
  LC_CardMgr_ResultCmdError
} LC_CARDMGR_RESULT;

#include "cardcontext_l.h"


LC_CARDMGR *LC_CardMgr_new(const GWEN_STRINGLIST *paths);
void LC_CardMgr_free(LC_CARDMGR *mgr);
void LC_CardMgr_Attach(LC_CARDMGR *mgr);

GWEN_MSGENGINE *LC_CardMgr_GetMsgEngine(const LC_CARDMGR *mgr);


LC_CARDCONTEXT *LC_CardMgr_SelectApp(LC_CARDMGR *mgr,
                                     const char *appName);

GWEN_XMLNODE *LC_CardMgr_FindAppNode(LC_CARDMGR *mgr,
                                     const char *appName);


#endif /* CHIPCARD_CLIENT_CARDMGR_L_H */


