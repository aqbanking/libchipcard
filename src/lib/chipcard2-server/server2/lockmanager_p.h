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



#ifndef CHIPCARD_SERVER2_LOCKMANAGER_P_H
#define CHIPCARD_SERVER2_LOCKMANAGER_P_H


#include "lockmanager_l.h"
#include "lockrequest_l.h"


struct LCS_LOCKMANAGER {
  LCS_LOCKREQUEST_LIST *requestList;
  LCS_LOCKREQUEST *currentRequest;
  char *what;
};

LCS_LOCKREQUEST*
LCS_LockManager_FindRequestByRequestId(LCS_LOCKMANAGER *lm,
                                       GWEN_TYPE_UINT32 rqid);

int LCS_LockManager_CountClientRequests(const LCS_LOCKMANAGER *lm,
                                        GWEN_TYPE_UINT32 clid);

#endif
