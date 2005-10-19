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


#include "connection_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/inherit.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_INHERIT(GWEN_NETCONNECTION, LCS_CONNECTION)



void LCS_Connection_TakeOver(GWEN_NETCONNECTION *conn){
  LCS_CONNECTION *sc;

  GWEN_NEW_OBJECT(LCS_CONNECTION, sc);
  DBG_MEM_INC("LCS_CONNECTION", 0);
  GWEN_INHERIT_SETDATA(GWEN_NETCONNECTION, LCS_CONNECTION,
                       conn, sc,
                       LCS_Connection_FreeData);
}



void LCS_Connection_SetType(GWEN_NETCONNECTION *conn,
                            LCS_CONNECTION_TYPE t){
  LCS_CONNECTION *sc;

  assert(conn);
  sc=GWEN_INHERIT_GETDATA(GWEN_NETCONNECTION, LCS_CONNECTION, conn);
  assert(sc);

  sc->type=t;
}



LCS_CONNECTION_TYPE LCS_Connection_GetType(const GWEN_NETCONNECTION *conn) {
  LCS_CONNECTION *sc;

  assert(conn);
  sc=GWEN_INHERIT_GETDATA(GWEN_NETCONNECTION, LCS_CONNECTION, conn);
  assert(sc);

  return sc->type;
}



int LCS_Connection_IsOfType(GWEN_NETCONNECTION *conn) {
  return GWEN_INHERIT_ISOFTYPE(GWEN_NETCONNECTION, LCS_CONNECTION, conn);
}



void LCS_Connection_SetServer(GWEN_NETCONNECTION *conn,
                              LCS_SERVER *cs){
  LCS_CONNECTION *sc;

  assert(conn);
  sc=GWEN_INHERIT_GETDATA(GWEN_NETCONNECTION, LCS_CONNECTION, conn);
  assert(sc);

  sc->server=cs;
}



LCS_SERVER *LCS_Connection_GetServer(const GWEN_NETCONNECTION *conn) {
  LCS_CONNECTION *sc;

  assert(conn);
  sc=GWEN_INHERIT_GETDATA(GWEN_NETCONNECTION, LCS_CONNECTION, conn);
  assert(sc);

  return sc->server;
}



void LCS_Connection_FreeData(void *bp, void *p) {
  LCS_CONNECTION *sc;

  sc=(LCS_CONNECTION*)p;

  GWEN_FREE_OBJECT(sc);
  DBG_MEM_DEC("LCS_CONNECTION");
}



