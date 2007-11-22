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



#ifndef CHIPCARD_SERVER_CL_CLIENT_L_H
#define CHIPCARD_SERVER_CL_CLIENT_L_H

typedef struct LCCL_CLIENT  LCCL_CLIENT;


#include <gwenhywfar/misc.h>


GWEN_LIST_FUNCTION_DEFS(LCCL_CLIENT, LCCL_Client);



LCCL_CLIENT *LCCL_Client_new(uint32_t id);
void LCCL_Client_free(LCCL_CLIENT *cl);
void LCCL_Client_Attach(LCCL_CLIENT *cl);

uint32_t LCCL_Client_GetClientId(const LCCL_CLIENT *cl);

int LCCL_Client_HasService(const LCCL_CLIENT *cl, uint32_t id);
int LCCL_Client_AddService(LCCL_CLIENT *cl, uint32_t id);
int LCCL_Client_DelService(LCCL_CLIENT *cl, uint32_t id);

int LCCL_Client_AddReader(LCCL_CLIENT *cl, uint32_t id);
int LCCL_Client_DelReader(LCCL_CLIENT *cl, uint32_t id);
uint32_t LCCL_Client_GetFirstReader(LCCL_CLIENT *cl);
uint32_t LCCL_Client_GetNextReader(LCCL_CLIENT *cl);




uint32_t LCCL_Client_GetWaitRequestCount(const LCCL_CLIENT *cl);
void LCCL_Client_AddWaitRequestCount(LCCL_CLIENT *cl);
void LCCL_Client_SubWaitRequestCount(LCCL_CLIENT *cl);
void LCCL_Client_ResetRequestCount(LCCL_CLIENT *cl);

const char *LCCL_Client_GetApplicationName(const LCCL_CLIENT *cl);
void LCCL_Client_SetApplicationName(LCCL_CLIENT *cl, const char *s);

const char *LCCL_Client_GetUserName(const LCCL_CLIENT *cl);
void LCCL_Client_SetUserName(LCCL_CLIENT *cl, const char *s);

uint32_t LCCL_Client_GetNotifyFlags(const LCCL_CLIENT *cl);
void LCCL_Client_SetNotifyFlags(LCCL_CLIENT *cl,
                                uint32_t flags);
void LCCL_Client_AddNotifyFlags(LCCL_CLIENT *cl,
                                uint32_t flags);
void LCCL_Client_DelNotifyFlags(LCCL_CLIENT *cl,
                                uint32_t flags);

uint32_t LCCL_Client_GetNotifyMask(const LCCL_CLIENT *cl);
void LCCL_Client_SetNotifyMask(LCCL_CLIENT *cl,
                               uint32_t mask);
void LCCL_Client_AddNotifyMask(LCCL_CLIENT *cl,
                               uint32_t mask);
void LCCL_Client_DelNotifyMask(LCCL_CLIENT *cl,
                               uint32_t mask);

int LCCL_Client_GetMaxClientLockTime(const LCCL_CLIENT *cl);
void LCCL_Client_SetMaxClientLockTime(LCCL_CLIENT *cl, int i);

int LCCL_Client_GetMaxClientLocks(const LCCL_CLIENT *cl);
void LCCL_Client_SetMaxClientLocks(LCCL_CLIENT *cl, int i);

int LCCL_Client_GetWantDestroy(const LCCL_CLIENT *cl);
void LCCL_Client_SetWantDestroy(LCCL_CLIENT *cl, int i);


void LCCL_Client_Dump(const LCCL_CLIENT *cl, FILE *f, int indent);

#endif /* CHIPCARD_SERVER_CL_CLIENT_L_H */





