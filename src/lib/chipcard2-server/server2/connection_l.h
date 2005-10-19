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


#ifndef CHIPCARD_SERVER2_CONN_L_H
#define CHIPCARD_SERVER2_CONN_L_H

#include <gwenhywfar/netconnection.h>


typedef enum {
  LCS_Connection_TypeUnknown=0,
  LCS_Connection_TypeDriver,
  LCS_Connection_TypeClient,
  LCS_Connection_TypeService
} LCS_CONNECTION_TYPE;

#include "server_l.h"


void LCS_Connection_TakeOver(GWEN_NETCONNECTION *conn);
void LCS_Connection_SetType(GWEN_NETCONNECTION *conn,
                            LCS_CONNECTION_TYPE t);
LCS_CONNECTION_TYPE LCS_Connection_GetType(const GWEN_NETCONNECTION *conn);

int LCS_Connection_IsOfType(GWEN_NETCONNECTION *conn);

void LCS_Connection_SetServer(GWEN_NETCONNECTION *conn,
                              LCS_SERVER *cs);
LCS_SERVER *LCS_Connection_GetServer(const GWEN_NETCONNECTION *conn);


#endif /* CHIPCARD_SERVER2_CONN_L_H */

