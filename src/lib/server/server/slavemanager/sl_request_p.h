/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: cl_request_p.h 211 2006-09-07 23:57:04Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/



#ifndef CHIPCARD_SERVER2_SL_REQUEST_P_H
#define CHIPCARD_SERVER2_SL_REQUEST_P_H


#include "sl_request_l.h"


struct LCSL_REQUEST {
  LCCO_CARD *card;
  LCCO_READER *reader;
  LCSL_SLAVEMANAGER *slaveManager;
  GWEN_TYPE_UINT32 uint32Data1;
  GWEN_TYPE_UINT32 uint32Data2;
};

static void GWENHYWFAR_CB LCSL_Request_FreeData(void *bp, void *p);



#endif /* CHIPCARD_SERVER2_SL_REQUEST_P_H */


