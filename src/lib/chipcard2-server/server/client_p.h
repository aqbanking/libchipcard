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



#ifndef CHIPCARD_SERVER_CLIENT_P_H
#define CHIPCARD_SERVER_CLIENT_P_H

#include <gwenhywfar/idlist.h>
#include <chipcard2-server/server/client.h>


struct LC_CLIENT {
  GWEN_LIST_ELEMENT(LC_CLIENT);
  GWEN_IDLIST *seenCards;
  GWEN_IDLIST *selectedReaders;
  GWEN_IDLIST *openServices;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 waitRequestCount;
  GWEN_TYPE_UINT32 lastWaitRequestId;

  char *appName;
  char *userName;

  GWEN_TYPE_UINT32 waitReaderFlags;
  GWEN_TYPE_UINT32 waitReaderMask;

  GWEN_TYPE_UINT32 notifyFlags;
  GWEN_TYPE_UINT32 notifyMask;

};



#endif /* CHIPCARD_SERVER_CLIENT_P_H */





