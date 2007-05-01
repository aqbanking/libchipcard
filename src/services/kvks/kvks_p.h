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


#ifndef CHIPCARD_SERVICE_KVK_P_H
#define CHIPCARD_SERVICE_KVK_P_H


#include <chipcard3/chipcard3.h>
#include <chipcard3/client/service/service.h>
#include <chipcard3/client/card.h>


#define SERVICE_KVK_ERROR_UNKNOWN_COMMAND        1


typedef struct SERVICE_KVK SERVICE_KVK;
struct SERVICE_KVK {
  LC_CARD_LIST2 *cards;
};


LC_CLIENT *ServiceKVK_new(int argc, char **argv);
int ServiceKVK_Start(LC_CLIENT *sv);


void GWENHYWFAR_CB ServiceKVK_freeData(void *bp, void *p);


const char *ServiceKVK_GetErrorText(LC_CLIENT *cl, GWEN_TYPE_UINT32 err);


GWEN_TYPE_UINT32 ServiceKVK_Command(LC_CLIENT *cl,
			    LC_SERVICECLIENT *scl,
			    GWEN_DB_NODE *dbRequest,
			    GWEN_DB_NODE *dbResponse);

int ServiceKVK_Work(LC_CLIENT *cl);


int ServiceKVK_NewCard(LC_CLIENT *cl, LC_CARD *cd);
int ServiceKVK_HandleCard(LC_CLIENT *cl, LC_CARD *cd);

GWEN_TYPE_UINT32 ServiceKVK_SendReadBinary(LC_CLIENT *cl, LC_CARD *cd,
                                           int offset, int size);
LC_CLIENT_RESULT ServiceKVK_CheckReadBinary(LC_CLIENT *cl, LC_CARD *cd);
LC_CLIENT_RESULT ServiceKVK_CalcTagSize(LC_CLIENT *cl, LC_CARD *cd,
                                        unsigned int *tagSize);
LC_CLIENT_RESULT ServiceKVK_HandleData(LC_CLIENT *cl, LC_CARD *cd);

int ServiceKVK_StoreCardData(LC_CLIENT *cl, LC_CARD *cd);



#endif /* CHIPCARD_SERVICE_KVK_H */






