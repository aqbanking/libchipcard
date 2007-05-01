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


#ifndef CHIPCARD_SERVER2_CONN_P_H
#define CHIPCARD_SERVER2_CONN_P_H

#include "connection_l.h"


typedef struct LCS_CONNECTION LCS_CONNECTION;
struct LCS_CONNECTION {
  LCS_CONNECTION_TYPE type;
  LCS_SERVER *server;
};


static
void GWENHYWFAR_CB LCS_Connection_FreeData(void *bp, void *p);


#endif /* CHIPCARD_SERVER2_CONN_P_H */

