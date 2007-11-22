/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: notifications_p.h 2 2005-01-02 10:05:37Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CLIENT_NOTIFICATIONS_P_H
#define CHIPCARD_CLIENT_NOTIFICATIONS_P_H

#include "notifications_l.h"


struct LC_NOTIFICATION {
  GWEN_INHERIT_ELEMENT(LC_NOTIFICATION)
  GWEN_LIST_ELEMENT(LC_NOTIFICATION)
  uint32_t serverId;
  char *clientId;
  char *ntype;
  char *ncode;
  GWEN_DB_NODE *data;
};



#endif /* CHIPCARD_CLIENT_NOTIFICATIONS_P_H */

