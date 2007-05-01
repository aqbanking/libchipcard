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



#ifndef CHIPCARD_SERVER2_CL_REQUEST_P_H
#define CHIPCARD_SERVER2_CL_REQUEST_P_H


#include "cl_request_l.h"


struct LCCL_REQUEST {
  LCCO_CARD *card;
  LCCL_CLIENTMANAGER *clientManager;
  LCCL_CLIENT *client;
  GWEN_TYPE_UINT32 uint32Data1;
  GWEN_TYPE_UINT32 uint32Data2;
};

void GWENHYWFAR_CB LCCL_Request_FreeData(void *bp, void *p);



#endif /* CHIPCARD_SERVER2_CL_REQUEST_P_H */


