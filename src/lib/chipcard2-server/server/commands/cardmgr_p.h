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


#ifndef CHIPCARD_SERVER_CARDMGR_P_H
#define CHIPCARD_SERVER_CARDMGR_P_H

#include <gwenhywfar/types.h>
#include <gwenhywfar/xml.h>

#include "cardmgr_l.h"
#include "cardcontext_l.h"


struct LC_CARDMGR {
  LC_CARDCONTEXT_LIST *contexts;
  GWEN_STRINGLIST *paths;
  GWEN_STRINGLIST *cardFiles;
  GWEN_STRINGLIST *appFiles;
  GWEN_STRINGLIST *loadedCards;
  GWEN_XMLNODE *xmlCards;
  GWEN_XMLNODE *xmlApps;
  GWEN_TYPE_UINT32 usage;
  GWEN_MSGENGINE *msgEngine;
};


void LC_CardMgr__SampleFiles(LC_CARDMGR *mgr,
                             const char *where);

void LC_CardMgr_SampleFiles(LC_CARDMGR *mgr,
                            const GWEN_STRINGLIST *sl);

int LC_CardMgr_MergeXMLDefs(LC_CARDMGR *mgr,
                            GWEN_XMLNODE *destNode,
                            GWEN_XMLNODE *node);


int LC_CardMgr_LoadCard(LC_CARDMGR *mgr, const char *name);
int LC_CardMgr_LoadAllCards(LC_CARDMGR *mgr);


#endif /* CHIPCARD_SERVER_CARDMGR_P_H */


