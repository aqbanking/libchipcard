/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: request_p.h 30 2005-01-26 00:16:06Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CLIENT_REQUEST_P_H
#define CHIPCARD_CLIENT_REQUEST_P_H


#include "request_l.h"




struct LC_REQUEST {
  GWEN_LIST_ELEMENT(LC_REQUEST);
  GWEN_DB_NODE *requestData;
  time_t requestTime;
  GWEN_TYPE_UINT32 requestId;
  GWEN_TYPE_UINT32 ipcRequestId;
  GWEN_TYPE_UINT32 serverId;
  int aborted;
  LC_CARD *card;
};



#endif /* CHIPCARD_CLIENT_REQUEST_P_H */



