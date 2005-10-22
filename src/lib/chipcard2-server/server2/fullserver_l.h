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



#ifndef CHIPCARD_SERVER2_FULLSERVER_L_H
#define CHIPCARD_SERVER2_FULLSERVER_L_H


#include "server_l.h"
#include "clientmanager/clientmanager_l.h"
#include "cardmanager/cardmanager_l.h"
#include "commandmanager/commandmanager_l.h"
#include "servicemanager/servicemanager_l.h"



LCS_SERVER *LCS_FullServer_new();

int LCS_FullServer_Init(LCS_SERVER *cs, GWEN_DB_NODE *db);
int LCS_FullServer_Fini(LCS_SERVER *cs, GWEN_DB_NODE *db);


LCCM_CARDMANAGER *LCS_FullServer_GetCardManager(const LCS_SERVER *cs);
void LCS_FullServer_SetCardManager(LCS_SERVER *cs, LCCM_CARDMANAGER *cm);

LCCMD_COMMANDMANAGER *LCS_FullServer_GetCommandManager(const LCS_SERVER *cs);
void LCS_FullServer_SetCommandManager(LCS_SERVER *cs,
                                      LCCMD_COMMANDMANAGER *cm);

LCSV_SERVICEMANAGER *LCS_FullServer_GetServiceManager(const LCS_SERVER *cs);
void LCS_FullServer_SetServiceManager(LCS_SERVER *cs,
                                      LCSV_SERVICEMANAGER *svm);

/** @return 1 if something could be done */
int LCS_FullServer_Work(LCS_SERVER *cs);

int LCS_FullServer_GetClientCount(LCS_SERVER *cs);

#endif /* CHIPCARD_SERVER2_FULLSERVER_L_H */

