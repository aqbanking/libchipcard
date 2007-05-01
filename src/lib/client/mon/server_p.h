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


#ifndef LC_MON_SERVER_P_H
#define LC_MON_SERVER_P_H


#include "server_l.h"

struct LCM_SERVER {
  GWEN_LIST_ELEMENT(LCM_SERVER)
  GWEN_TYPE_UINT32 serverId;
  char *clientId;
  LCM_DRIVER_LIST *drivers;
  LCM_READER_LIST *readers;
  LCM_SERVICE_LIST *services;
};


#endif

