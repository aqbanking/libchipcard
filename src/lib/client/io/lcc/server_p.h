/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: server_p.h 2 2005-01-02 10:05:37Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CLIENT_SERVER_P_H
#define CHIPCARD_CLIENT_SERVER_P_H

#define LC_SERVER_STARTTIMEOUT 20

#include <gwenhywfar/misc.h>
#include "server_l.h"


struct LC_SERVER {
  GWEN_LIST_ELEMENT(LC_SERVER);
  GWEN_TYPE_UINT32 serverId;
  LC_SERVER_STATUS status;
  GWEN_TYPE_UINT32 currentCommand;
};




#endif /* CHIPCARD_CLIENT_SERVER_P_H */





