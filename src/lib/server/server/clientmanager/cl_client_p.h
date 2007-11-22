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
  uint32_t clientId;
  uint32_t waitRequestCount;
  uint32_t lastWaitRequestId;

  int maxClientLockTime;
  int maxClientLocks;

  char *appName;
  char *userName;

  uint32_t notifyFlags;
  uint32_t notifyMask;

  uint32_t usage;

  int destroy;
};



#endif /* CHIPCARD_SERVER_CL_CLIENT_P_H */





