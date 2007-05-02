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


#ifndef CHIPCARD_CT_DDV_P_H
#define CHIPCARD_CT_DDV_P_H


#include <gwenhywfar/crypttoken.h>
#include <chipcard3/client/card.h>


typedef struct LC_CT_PLUGIN_DDV LC_CT_PLUGIN_DDV;

struct LC_CT_PLUGIN_DDV {
  LC_CLIENT *client;
};

GWEN_PLUGIN *LC_CryptTokenDDV_Plugin_new(GWEN_PLUGIN_MANAGER *pm,
                                         const char *modName,
                                         const char *fileName);

void GWENHYWFAR_CB LC_CryptTokenDDV_Plugin_FreeData(void *bp, void *p);




typedef struct LC_CT_DDV LC_CT_DDV;

struct LC_CT_DDV {
  GWEN_PLUGIN_MANAGER *pluginManager;
  LC_CLIENT *client;
  LC_CARD *card;
};

GWEN_CRYPTTOKEN *LC_CryptTokenDDV_new(GWEN_PLUGIN_MANAGER *pm,
                                      LC_CLIENT *lc,
                                      const char *name);

void GWENHYWFAR_CB LC_CryptTokenDDV_FreeData(void *bp, void *p);

int LC_CryptTokenDDV__GetCard(GWEN_CRYPTTOKEN *ct,
			      int manage);

int LC_CryptTokenDDV__EnterPin(GWEN_CRYPTTOKEN *ct,
			       LC_CARD *hcard,
			       GWEN_CRYPTTOKEN_PINTYPE pt);


int LC_CryptTokenDDV_Open(GWEN_CRYPTTOKEN *ct, int manage);
int LC_CryptTokenDDV_Create(GWEN_CRYPTTOKEN *ct);
int LC_CryptTokenDDV_Close(GWEN_CRYPTTOKEN *ct);


int LC_CryptTokenDDV_Sign(GWEN_CRYPTTOKEN *ct,
                          const GWEN_CRYPTTOKEN_CONTEXT *ctx,
                          const char *ptr,
                          unsigned int len,
                          GWEN_BUFFER *dst);
int LC_CryptTokenDDV_Verify(GWEN_CRYPTTOKEN *ct,
                            const GWEN_CRYPTTOKEN_CONTEXT *ctx,
                            const char *ptr,
                            unsigned int len,
                            const char *sigptr,
                            unsigned int siglen);
int LC_CryptTokenDDV_Encrypt(GWEN_CRYPTTOKEN *ct,
                             const GWEN_CRYPTTOKEN_CONTEXT *ctx,
                             const char *ptr,
                             unsigned int len,
                             GWEN_BUFFER *dst);
int LC_CryptTokenDDV_Decrypt(GWEN_CRYPTTOKEN *ct,
                             const GWEN_CRYPTTOKEN_CONTEXT *ctx,
                             const char *ptr,
                             unsigned int len,
                             GWEN_BUFFER *dst);

int LC_CryptTokenDDV_GetSignSeq(GWEN_CRYPTTOKEN *ct,
                                GWEN_TYPE_UINT32 kid,
                                GWEN_TYPE_UINT32 *signSeq);

int LC_CryptTokenDDV_ReadKeySpec(GWEN_CRYPTTOKEN *ct,
                                 GWEN_TYPE_UINT32 kid,
                                 GWEN_KEYSPEC **ks);

int LC_CryptTokenDDV_FillUserList(GWEN_CRYPTTOKEN *ct,
                                  GWEN_CRYPTTOKEN_USER_LIST *ul);

int LC_CryptTokenDDV_GetTokenIdData(GWEN_CRYPTTOKEN *ct, GWEN_BUFFER *buf);


GWEN_CRYPTTOKEN *LC_CryptTokenDDV_Plugin_CreateToken(GWEN_PLUGIN *pl,
                                                     const char *subTypeName,
                                                     const char *name);

GWEN_PLUGIN *GWEN_CryptTokenDDV_Plugin_new(GWEN_PLUGIN_MANAGER *pm,
                                           const char *modName,
                                           const char *fileName);

GWEN_CRYPTTOKEN *LC_CryptTokenDDV_Plugin_CreateToken(GWEN_PLUGIN *pl,
                                                     const char *subTypeName,
                                                     const char *name);

int LC_CryptTokenDDV_Plugin_CheckToken(GWEN_PLUGIN *pl,
				       GWEN_BUFFER *subTypeName,
				       GWEN_BUFFER *name);

int LC_CryptTokenDDV__IncSignSeq(GWEN_CRYPTTOKEN *ct,
                                 GWEN_TYPE_UINT32 kid);


#endif

