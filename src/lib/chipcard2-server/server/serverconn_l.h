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


#ifndef CHIPCARD_SERVER_SERVERCONN_L_H
#define CHIPCARD_SERVER_SERVERCONN_L_H

#include <gwenhywfar/netconnection.h>
#include "cardserver_l.h"
#include <chipcard2-server/server/service.h>


typedef enum {
  LC_ServerConn_TypeUnknown=0,
  LC_ServerConn_TypeDriver,
  LC_ServerConn_TypeClient,
  LC_ServerConn_TypeService
} LC_SERVERCONN_TYPE;



void LC_ServerConn_TakeOver(GWEN_NETCONNECTION *conn);
void LC_ServerConn_SetType(GWEN_NETCONNECTION *conn,
                           LC_SERVERCONN_TYPE t);
LC_SERVERCONN_TYPE LC_ServerConn_GetType(const GWEN_NETCONNECTION *conn);

int LC_ServerConn_IsOfType(GWEN_NETCONNECTION *conn);

void LC_ServerConn_SetDriver(GWEN_NETCONNECTION *conn,
                             LC_DRIVER *d);
LC_DRIVER *LC_ServerConn_GetDriver(const GWEN_NETCONNECTION *conn);

void LC_ServerConn_SetService(GWEN_NETCONNECTION *conn,
                              LC_SERVICE *as);
LC_SERVICE *LC_ServerConn_GetService(const GWEN_NETCONNECTION *conn);

void LC_ServerConn_SetCardServer(GWEN_NETCONNECTION *conn,
                                 LC_CARDSERVER *cs);
LC_CARDSERVER *LC_ServerConn_GetCardServer(const GWEN_NETCONNECTION *conn);


#endif /* CHIPCARD_SERVER_SERVERCONN_L_H */

