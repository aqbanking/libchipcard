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


#ifndef CHIPCARD_SERVER_REQUEST_P_H
#define CHIPCARD_SERVER_REQUEST_P_H


#include <chipcard2-server/server/request.h>


struct LC_REQUEST {
  GWEN_LIST_ELEMENT(LC_REQUEST);
  LC_CLIENT *client;
  GWEN_TYPE_UINT32 inRequestId;
  GWEN_DB_NODE *inRequestData;
  GWEN_TYPE_UINT32 outRequestId;
  GWEN_DB_NODE *outRequestData;
  LC_CARD *card;
  GWEN_TYPE_UINT32 requestId;
  GWEN_TYPE_UINT32 usage;

};



#endif /* CHIPCARD_SERVER_REQUEST_P_H */


