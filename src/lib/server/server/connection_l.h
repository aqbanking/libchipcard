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

#include <gwenhywfar/netlayer.h>


typedef enum {
  LCS_Connection_Type_Unknown=0,
  LCS_Connection_Type_Driver,
  LCS_Connection_Type_Client,
  LCS_Connection_Type_Service,
  LCS_Connection_Type_Master
} LCS_CONNECTION_TYPE;

#include "server_l.h"


void LCS_Connection_TakeOver(GWEN_NETLAYER *conn);
void LCS_Connection_SetType(GWEN_NETLAYER *conn,
                            LCS_CONNECTION_TYPE t);
LCS_CONNECTION_TYPE LCS_Connection_GetType(const GWEN_NETLAYER *conn);

int LCS_Connection_IsOfType(GWEN_NETLAYER *conn);

void LCS_Connection_SetServer(GWEN_NETLAYER *conn,
                              LCS_SERVER *cs);
LCS_SERVER *LCS_Connection_GetServer(const GWEN_NETLAYER *conn);


#endif /* CHIPCARD_SERVER2_CONN_L_H */
