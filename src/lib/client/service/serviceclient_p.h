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



#ifndef CHIPCARD_SERVICE_CLIENT_P_H
#define CHIPCARD_SERVICE_CLIENT_P_H

#include "serviceclient.h"


struct LC_SERVICECLIENT {
  GWEN_LIST_ELEMENT(LC_SERVICECLIENT)
  GWEN_INHERIT_ELEMENT(LC_SERVICECLIENT)
  GWEN_TYPE_UINT32 clientId;

  char *userName;
};



#endif /* CHIPCARD_SERVICE_CLIENT_P_H */





