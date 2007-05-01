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


#ifndef CHIPCARD_CT_STARCOS_P_H
#define CHIPCARD_CT_STARCOS_P_H


#include <gwenhywfar/crypttoken.h>
#include <chipcard3/client/card.h>


typedef struct LC_CT_PLUGIN_STARCOS LC_CT_PLUGIN_STARCOS;

struct LC_CT_PLUGIN_STARCOS {
  LC_CLIENT *client;
};

GWEN_PLUGIN *LC_CryptTokenSTARCOS_Plugin_new(GWEN_PLUGIN_MANAGER *pm,
                                         const char *modName,
                                         const char *fileName);

void GWENHYWFAR_CB LC_CryptTokenSTARCOS_Plugin_FreeData(void *bp, void *p);




typedef struct LC_CT_STARCOS LC_CT_STARCOS;

struct LC_CT_STARCOS {
  GWEN_PLUGIN_MANAGER *pluginManager;
  LC_CLIENT *client;
  LC_CARD *card;
  int haveChPin;
  int haveEgPin;
  int needClose;
};

GWEN_CRYPTTOKEN *LC_CryptTokenSTARCOS_new(GWEN_PLUGIN_MANAGER *pm,
                                          LC_CLIENT *lc,
                                          const char *subTypeName,
                                          const char *name);

void GWENHYWFAR_CB LC_CryptTokenSTARCOS_FreeData(void *bp, void *p);

int LC_CryptTokenSTARCOS__GetCard(GWEN_CRYPTTOKEN *ct,
                                  int manage);

/**
 * pids: CardHolder Pin=0x90, Gateway Pin=0x91
 */

int LC_CryptTokenSTARCOS__Open(GWEN_CRYPTTOKEN *ct, int manage);


int LC_CryptTokenSTARCOS_Open(GWEN_CRYPTTOKEN *ct, int manage);
int LC_CryptTokenSTARCOS_Create(GWEN_CRYPTTOKEN *ct);
int LC_CryptTokenSTARCOS_Close(GWEN_CRYPTTOKEN *ct);

/**
 * @return -1 on error, 0 if algo to be omitted
 */
int LC_CryptTokenSTARCOS__TransformAlgo(int tmpl,
                                        GWEN_CRYPTTOKEN_HASHALGO hashAlgo,
                                        GWEN_CRYPTTOKEN_PADDALGO paddAlgo);


int LC_CryptTokenSTARCOS_Sign(GWEN_CRYPTTOKEN *ct,
                              const GWEN_CRYPTTOKEN_CONTEXT *ctx,
                              const char *ptr,
                              unsigned int len,
                              GWEN_BUFFER *dst);
int LC_CryptTokenSTARCOS_Verify(GWEN_CRYPTTOKEN *ct,
                                const GWEN_CRYPTTOKEN_CONTEXT *ctx,
                                const char *ptr,
                                unsigned int len,
                                const char *sigptr,
                                unsigned int siglen);
int LC_CryptTokenSTARCOS_Encrypt(GWEN_CRYPTTOKEN *ct,
                                 const GWEN_CRYPTTOKEN_CONTEXT *ctx,
                                 const char *ptr,
                                 unsigned int len,
                                 GWEN_BUFFER *dst);
int LC_CryptTokenSTARCOS_Decrypt(GWEN_CRYPTTOKEN *ct,
                                 const GWEN_CRYPTTOKEN_CONTEXT *ctx,
                                 const char *ptr,
                                 unsigned int len,
                                 GWEN_BUFFER *dst);

int LC_CryptTokenSTARCOS_GetSignSeq(GWEN_CRYPTTOKEN *ct,
                                    GWEN_TYPE_UINT32 kid,
                                    GWEN_TYPE_UINT32 *signSeq);

int LC_CryptTokenSTARCOS_ReadKeySpec(GWEN_CRYPTTOKEN *ct,
                                     GWEN_TYPE_UINT32 kid,
                                     GWEN_KEYSPEC **ks);

int LC_CryptTokenSTARCOS_FillUserList(GWEN_CRYPTTOKEN *ct,
                                      GWEN_CRYPTTOKEN_USER_LIST *ul);

int LC_CryptTokenSTARCOS_GetTokenIdData(GWEN_CRYPTTOKEN *ct,
                                        GWEN_BUFFER *buf);


GWEN_CRYPTTOKEN*
  LC_CryptTokenSTARCOS_Plugin_CreateToken(GWEN_PLUGIN *pl,
                                          const char *subTypeName,
                                          const char *name);

GWEN_PLUGIN*
  GWEN_CryptTokenSTARCOS_Plugin_new(GWEN_PLUGIN_MANAGER *pm,
                                    const char *modName,
                                    const char *fileName);

GWEN_CRYPTTOKEN*
  LC_CryptTokenSTARCOS_Plugin_CreateToken(GWEN_PLUGIN *pl,
                                          const char *subTypeName,
                                          const char *name);

int LC_CryptTokenSTARCOS_Plugin_CheckToken(GWEN_PLUGIN *pl,
                                           GWEN_BUFFER *subTypeName,
                                           GWEN_BUFFER *name);

int LC_CryptTokenSTARCOS__IncSignSeq(GWEN_CRYPTTOKEN *ct,
                                     GWEN_TYPE_UINT32 kid);

int LC_CryptTokenSTARCOS_ReadKey(GWEN_CRYPTTOKEN *ct,
                                 GWEN_TYPE_UINT32 kid,
                                 GWEN_CRYPTKEY **key);

int LC_CryptTokenSTARCOS_WriteKey(GWEN_CRYPTTOKEN *ct,
                                  GWEN_TYPE_UINT32 kid,
                                  const GWEN_CRYPTKEY *key);

int LC_CryptTokenSTARCOS_ReadKeySpec(GWEN_CRYPTTOKEN *ct,
                                     GWEN_TYPE_UINT32 kid,
                                     GWEN_KEYSPEC **ks);

int LC_CryptTokenSTARCOS_WriteKeySpec(GWEN_CRYPTTOKEN *ct,
                                      GWEN_TYPE_UINT32 kid,
                                      const GWEN_KEYSPEC *ks);


int LC_CryptTokenSTARCOS_GenerateKey(GWEN_CRYPTTOKEN *ct,
                                     const GWEN_CRYPTTOKEN_KEYINFO *ki,
                                     GWEN_CRYPTKEY **key);

int LC_CryptTokenSTARCOS_Status_toCtStatus(int i);
int LC_CryptTokenSTARCOS_Status_fromCtStatus(int i);


int LC_CryptTokenSTARCOS__VerifyPin(GWEN_CRYPTTOKEN *ct,
                                    LC_CARD *hcard,
                                    GWEN_CRYPTTOKEN_PINTYPE pt);

int LC_CryptTokenSTARCOS__ChangePin(GWEN_CRYPTTOKEN *ct,
                                    LC_CARD *hcard,
                                    GWEN_CRYPTTOKEN_PINTYPE pt);

int LC_CryptTokenSTARCOS_ChangePin(GWEN_CRYPTTOKEN *ct,
                                   GWEN_CRYPTTOKEN_PINTYPE pt);
int LC_CryptTokenSTARCOS_VerifyPin(GWEN_CRYPTTOKEN *ct,
                                   GWEN_CRYPTTOKEN_PINTYPE pt);

#endif

