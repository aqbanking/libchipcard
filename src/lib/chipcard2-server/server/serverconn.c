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


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "serverconn_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/inherit.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_INHERIT(GWEN_NETCONNECTION, LC_SERVERCONN)



void LC_ServerConn_TakeOver(GWEN_NETCONNECTION *conn){
  LC_SERVERCONN *sc;

  GWEN_NEW_OBJECT(LC_SERVERCONN, sc);
  DBG_MEM_INC("LC_SERVERCONN", 0);
  GWEN_INHERIT_SETDATA(GWEN_NETCONNECTION, LC_SERVERCONN,
                       conn, sc,
                       LC_ServerConn_FreeData);
  //GWEN_NetConnection_SetCheckFn(conn, LC_ServerConn_Check);
}



void LC_ServerConn_SetType(GWEN_NETCONNECTION *conn,
                           LC_SERVERCONN_TYPE t){
  LC_SERVERCONN *sc;

  assert(conn);
  sc=GWEN_INHERIT_GETDATA(GWEN_NETCONNECTION, LC_SERVERCONN, conn);
  assert(sc);

  sc->type=t;
}



LC_SERVERCONN_TYPE LC_ServerConn_GetType(const GWEN_NETCONNECTION *conn) {
  LC_SERVERCONN *sc;

  assert(conn);
  sc=GWEN_INHERIT_GETDATA(GWEN_NETCONNECTION, LC_SERVERCONN, conn);
  assert(sc);

  return sc->type;
}


int LC_ServerConn_IsOfType(GWEN_NETCONNECTION *conn) {
  return GWEN_INHERIT_ISOFTYPE(GWEN_NETCONNECTION, LC_SERVERCONN, conn);
}


void LC_ServerConn_SetCardServer(GWEN_NETCONNECTION *conn,
                                 LC_CARDSERVER *cs){
  LC_SERVERCONN *sc;

  assert(conn);
  sc=GWEN_INHERIT_GETDATA(GWEN_NETCONNECTION, LC_SERVERCONN, conn);
  assert(sc);

  sc->cardServer=cs;
}



LC_CARDSERVER *LC_ServerConn_GetCardServer(const GWEN_NETCONNECTION *conn) {
  LC_SERVERCONN *sc;

  assert(conn);
  sc=GWEN_INHERIT_GETDATA(GWEN_NETCONNECTION, LC_SERVERCONN, conn);
  assert(sc);

  return sc->cardServer;
}



void LC_ServerConn_FreeData(void *bp, void *p) {
  LC_SERVERCONN *sc;

  sc=(LC_SERVERCONN*)p;

  GWEN_FREE_OBJECT(sc);
  DBG_MEM_DEC("LC_SERVERCONN");
}



GWEN_TYPE_UINT32 LC_ServerConn_Check(GWEN_NETCONNECTION *conn){
  LC_SERVERCONN *sc;

  assert(conn);
  sc=GWEN_INHERIT_GETDATA(GWEN_NETCONNECTION, LC_SERVERCONN, conn);
  assert(sc);

  assert(sc->cardServer);
  if (sc->type==LC_ServerConn_TypeDriver) {
    return LC_CardServer_CheckConnForDriver(sc->cardServer, sc->driver);
  }
  return 0;
}



void LC_ServerConn_SetDriver(GWEN_NETCONNECTION *conn, LC_DRIVER *d){
  LC_SERVERCONN *sc;

  assert(conn);
  sc=GWEN_INHERIT_GETDATA(GWEN_NETCONNECTION, LC_SERVERCONN, conn);
  assert(sc);

  sc->driver=d;
}



LC_DRIVER *LC_ServerConn_GetDriver(const GWEN_NETCONNECTION *conn){
  LC_SERVERCONN *sc;

  assert(conn);
  sc=GWEN_INHERIT_GETDATA(GWEN_NETCONNECTION, LC_SERVERCONN, conn);
  assert(sc);

  return sc->driver;
}



void LC_ServerConn_SetService(GWEN_NETCONNECTION *conn,
                              LC_SERVICE *as){
  LC_SERVERCONN *sc;

  assert(conn);
  sc=GWEN_INHERIT_GETDATA(GWEN_NETCONNECTION, LC_SERVERCONN, conn);
  assert(sc);

  sc->service=as;
}



LC_SERVICE *LC_ServerConn_GetService(const GWEN_NETCONNECTION *conn){
  LC_SERVERCONN *sc;

  assert(conn);
  sc=GWEN_INHERIT_GETDATA(GWEN_NETCONNECTION, LC_SERVERCONN, conn);
  assert(sc);

  return sc->service;
}















