/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: cl_request_l.h 211 2006-09-07 23:57:04Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/



#ifndef CHIPCARD_SERVER2_SL_REQUEST_L_H
#define CHIPCARD_SERVER2_SL_REQUEST_L_H



#include <gwenhywfar/request.h>
#include "slavemanager_l.h"
#include "common/card.h"


typedef struct LCSL_REQUEST LCSL_REQUEST;


GWEN_IPC_REQUEST *LCSL_Request_new();

LCCO_CARD *LCSL_Request_GetCard(const GWEN_IPC_REQUEST *rq);
void LCSL_Request_SetCard(GWEN_IPC_REQUEST *rq, LCCO_CARD *card);

LCCO_READER *LCSL_Request_GetReader(const GWEN_IPC_REQUEST *rq);
void LCSL_Request_SetReader(GWEN_IPC_REQUEST *rq, LCCO_READER *r);

LCSL_SLAVEMANAGER *LCSL_Request_GetSlaveManager(const GWEN_IPC_REQUEST *rq);
void LCSL_Request_SetSlaveManager(GWEN_IPC_REQUEST *rq,
                                  LCSL_SLAVEMANAGER *slm);

uint32_t LCSL_Request_GetUint32Data(const GWEN_IPC_REQUEST *rq);
void LCSL_Request_SetUint32Data(GWEN_IPC_REQUEST *rq, uint32_t i);

uint32_t LCSL_Request_GetUint32Data2(const GWEN_IPC_REQUEST *rq);
void LCSL_Request_SetUint32Data2(GWEN_IPC_REQUEST *rq, uint32_t i);



#endif /* CHIPCARD_SERVER2_CL_REQUEST_L_H */


