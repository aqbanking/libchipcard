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



#ifndef CHIPCARD_SERVER_CL_CLIENT_P_H
#define CHIPCARD_SERVER_CL_CLIENT_P_H

#include <gwenhywfar/idlist.h>
#include "cl_client_l.h"


struct LCCL_CLIENT {
  GWEN_LIST_ELEMENT(LCCL_CLIENT);
  GWEN_IDLIST *openServices;
  GWEN_IDLIST *usedReaders;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 waitRequestCount;
  GWEN_TYPE_UINT32 lastWaitRequestId;

  int maxClientLockTime;
  int maxClientLocks;

  char *appName;
  char *userName;

  GWEN_TYPE_UINT32 notifyFlags;
  GWEN_TYPE_UINT32 notifyMask;

  GWEN_TYPE_UINT32 usage;

  int destroy;
};



#endif /* CHIPCARD_SERVER_CL_CLIENT_P_H */





