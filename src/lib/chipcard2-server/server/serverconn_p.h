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


#ifndef CHIPCARD_SERVER_SERVERCONN_P_H
#define CHIPCARD_SERVER_SERVERCONN_P_H

#include "serverconn_l.h"


typedef struct LC_SERVERCONN LC_SERVERCONN;
struct LC_SERVERCONN {
  LC_SERVERCONN_TYPE type;
  LC_CARDSERVER *cardServer;
  LC_DRIVER *driver;
  LC_SERVICE *service;
};


void LC_ServerConn_FreeData(void *bp, void *p);
GWEN_TYPE_UINT32 LC_ServerConn_Check(GWEN_NETCONNECTION *conn);


#endif /* CHIPCARD_SERVER_SERVERCONN_P_H */

