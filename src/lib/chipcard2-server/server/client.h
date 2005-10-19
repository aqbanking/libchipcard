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



#ifndef CHIPCARD_SERVER_CLIENT_H
#define CHIPCARD_SERVER_CLIENT_H

typedef struct LC_CLIENT  LC_CLIENT;

#include <chipcard2/chipcard2.h>


#include <gwenhywfar/misc.h>


GWEN_LIST_FUNCTION_DEFS(LC_CLIENT, LC_Client);



LC_CLIENT *LC_Client_new(GWEN_TYPE_UINT32 id);
void LC_Client_free(LC_CLIENT *cl);

GWEN_TYPE_UINT32 LC_Client_GetClientId(const LC_CLIENT *cl);

int LC_Client_HasReader(const LC_CLIENT *cl, GWEN_TYPE_UINT32 id);
int LC_Client_AddReader(const LC_CLIENT *cl, GWEN_TYPE_UINT32 id);
int LC_Client_DelReader(const LC_CLIENT *cl, GWEN_TYPE_UINT32 id);

int LC_Client_HasCard(const LC_CLIENT *cl, GWEN_TYPE_UINT32 id);
int LC_Client_AddCard(LC_CLIENT *cl, GWEN_TYPE_UINT32 id);
int LC_Client_DelCard(LC_CLIENT *cl, GWEN_TYPE_UINT32 id);
void LC_Client_DelAllCards(LC_CLIENT *cl);

int LC_Client_HasService(const LC_CLIENT *cl, GWEN_TYPE_UINT32 id);
int LC_Client_AddService(LC_CLIENT *cl, GWEN_TYPE_UINT32 id);
int LC_Client_DelService(LC_CLIENT *cl, GWEN_TYPE_UINT32 id);


GWEN_TYPE_UINT32 LC_Client_GetWaitRequestCount(const LC_CLIENT *cl);
void LC_Client_AddWaitRequestCount(LC_CLIENT *cl);
void LC_Client_SubWaitRequestCount(LC_CLIENT *cl);

GWEN_TYPE_UINT32 LC_Client_GetLastWaitRequestId(const LC_CLIENT *cl);
void LC_Client_SetLastWaitRequestId(LC_CLIENT *cl, GWEN_TYPE_UINT32 id);

GWEN_TYPE_UINT32 LC_Client_GetWaitReaderFlags(const LC_CLIENT *cl);
GWEN_TYPE_UINT32 LC_Client_GetWaitReaderMask(const LC_CLIENT *cl);
void LC_Client_AddWaitReaderState(LC_CLIENT *cl,
                                  GWEN_TYPE_UINT32 flags,
                                  GWEN_TYPE_UINT32 mask);
void LC_Client_ResetWaitReaderState(LC_CLIENT *cl);

const char *LC_Client_GetApplicationName(const LC_CLIENT *cl);
void LC_Client_SetApplicationName(LC_CLIENT *cl, const char *s);

const char *LC_Client_GetUserName(const LC_CLIENT *cl);
void LC_Client_SetUserName(LC_CLIENT *cl, const char *s);


GWEN_TYPE_UINT32 LC_Client_GetNotifyFlags(const LC_CLIENT *cl);
void LC_Client_SetNotifyFlags(LC_CLIENT *cl,
                              GWEN_TYPE_UINT32 flags);
void LC_Client_AddNotifyFlags(LC_CLIENT *cl,
                              GWEN_TYPE_UINT32 flags);
void LC_Client_DelNotifyFlags(LC_CLIENT *cl,
                              GWEN_TYPE_UINT32 flags);

GWEN_TYPE_UINT32 LC_Client_GetNotifyMask(const LC_CLIENT *cl);
void LC_Client_SetNotifyMask(LC_CLIENT *cl,
                              GWEN_TYPE_UINT32 mask);
void LC_Client_AddNotifyMask(LC_CLIENT *cl,
                              GWEN_TYPE_UINT32 mask);
void LC_Client_DelNotifyMask(LC_CLIENT *cl,
                              GWEN_TYPE_UINT32 mask);



#endif /* CHIPCARD_SERVER_CLIENT_H */





