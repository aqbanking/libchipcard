/***************************************************************************
    begin       : Tue Jun 21 2011
    copyright   : (C) 2011 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CT_ZKA_P_H
#define CHIPCARD_CT_ZKA_P_H


#include <gwenhywfar/ct_be.h>
#include <gwenhywfar/ctplugin.h>
#include <chipcard/card.h>


#define LC_CT_ZKA_NUM_CONTEXT 31
#define LC_CT_ZKA_NUM_KEY     20


#define LC_CT_ZKA_USER_SIGN_KEY     1
#define LC_CT_ZKA_USER_CRYPT_KEY    2
#define LC_CT_ZKA_USER_AUTH_KEY     3

#define LC_CT_ZKA_PEER_SIGN_KEY     4
#define LC_CT_ZKA_PEER_CRYPT_KEY    5
#define LC_CT_ZKA_PEER_AUTH_KEY     6



typedef struct LC_CT_PLUGIN_ZKA LC_CT_PLUGIN_ZKA;

struct LC_CT_PLUGIN_ZKA {
  LC_CLIENT *client;
};

static void GWENHYWFAR_CB LC_Crypt_TokenZka_Plugin_FreeData(void *bp, void *p);




typedef struct LC_CT_ZKA LC_CT_ZKA;

struct LC_CT_ZKA {
  GWEN_PLUGIN_MANAGER *pluginManager;
  GWEN_CRYPT_TOKEN_KEYINFO *keyInfos[LC_CT_ZKA_NUM_KEY];
  GWEN_CRYPT_TOKEN_CONTEXT *contexts[LC_CT_ZKA_NUM_CONTEXT];
  LC_CLIENT *client;
  LC_CARD *card;
  int haveAccessPin;
  int haveAdminPin;
  int contextListIsValid;
};

static GWEN_CRYPT_TOKEN *LC_Crypt_TokenZka_new(GWEN_PLUGIN_MANAGER *pm,
						   LC_CLIENT *lc,
						   const char *name);

static void GWENHYWFAR_CB LC_Crypt_TokenZka_FreeData(void *bp, void *p);

static int LC_Crypt_TokenZka__GetCard(GWEN_CRYPT_TOKEN *ct,
					  uint32_t guiid);

static int GWENHYWFAR_CB
  LC_Crypt_TokenZka_GetKeyIdList(GWEN_CRYPT_TOKEN *ct,
				     uint32_t *pIdList,
				     uint32_t *pCount,
				     uint32_t gid);

static const GWEN_CRYPT_TOKEN_KEYINFO* GWENHYWFAR_CB
  LC_Crypt_TokenZka_GetKeyInfo(GWEN_CRYPT_TOKEN *ct,
				   uint32_t id,
				   uint32_t flags,
				   uint32_t gid);

static int GWENHYWFAR_CB
  LC_Crypt_TokenZka_SetKeyInfo(GWEN_CRYPT_TOKEN *ct,
				   uint32_t id,
				   const GWEN_CRYPT_TOKEN_KEYINFO *ki,
				   uint32_t gid);

static int GWENHYWFAR_CB
  LC_Crypt_TokenZka_GetContextIdList(GWEN_CRYPT_TOKEN *ct,
					 uint32_t *pIdList,
					 uint32_t *pCount,
					 uint32_t gid);

static const GWEN_CRYPT_TOKEN_CONTEXT* GWENHYWFAR_CB
  LC_Crypt_TokenZka_GetContext(GWEN_CRYPT_TOKEN *ct,
				   uint32_t id,
				   uint32_t gid);

static int GWENHYWFAR_CB
  LC_Crypt_TokenZka_SetContext(GWEN_CRYPT_TOKEN *ct,
				   uint32_t id,
				   const GWEN_CRYPT_TOKEN_CONTEXT *ctx,
				   uint32_t gid);


static int GWENHYWFAR_CB
  LC_Crypt_TokenZka_Sign(GWEN_CRYPT_TOKEN *ct,
			     uint32_t keyId,
			     GWEN_CRYPT_PADDALGO *a,
			     const uint8_t *pInData,
			     uint32_t inLen,
			     uint8_t *pSignatureData,
			     uint32_t *pSignatureLen,
			     uint32_t *pSeqCounter,
			     uint32_t gid);

static int GWENHYWFAR_CB
  LC_Crypt_TokenZka_Verify(GWEN_CRYPT_TOKEN *ct,
			       uint32_t keyId,
			       GWEN_CRYPT_PADDALGO *a,
			       const uint8_t *pInData,
			       uint32_t inLen,
			       const uint8_t *pSignatureData,
			       uint32_t signatureLen,
			       uint32_t seqCounter,
			       uint32_t gid);

static int GWENHYWFAR_CB
  LC_Crypt_TokenZka_Encipher(GWEN_CRYPT_TOKEN *ct,
				 uint32_t keyId,
				 GWEN_CRYPT_PADDALGO *a,
				 const uint8_t *pInData,
				 uint32_t inLen,
				 uint8_t *pOutData,
				 uint32_t *pOutLen,
				 uint32_t gid);

static int GWENHYWFAR_CB
  LC_Crypt_TokenZka_Decipher(GWEN_CRYPT_TOKEN *ct,
				 uint32_t keyId,
				 GWEN_CRYPT_PADDALGO *a,
				 const uint8_t *pInData,
				 uint32_t inLen,
				 uint8_t *pOutData,
				 uint32_t *pOutLen,
				 uint32_t gid);


static int GWENHYWFAR_CB
  LC_Crypt_TokenZka_Open(GWEN_CRYPT_TOKEN *ct, int admin, uint32_t guiid);
static int GWENHYWFAR_CB
  LC_Crypt_TokenZka_Close(GWEN_CRYPT_TOKEN *ct, int abandon, uint32_t guiid);




static GWEN_CRYPT_TOKEN* GWENHYWFAR_CB
  LC_Crypt_TokenZka_Plugin_CreateToken(GWEN_PLUGIN *pl,
                                       const char *name);

static GWEN_CRYPT_TOKEN* GWENHYWFAR_CB
  LC_Crypt_TokenZka_Plugin_CreateToken(GWEN_PLUGIN *pl,
                                       const char *name);

static int GWENHYWFAR_CB
  LC_Crypt_TokenZka_Plugin_CheckToken(GWEN_PLUGIN *pl,
                                      GWEN_BUFFER *name);

static GWEN_PLUGIN *LC_Crypt_TokenZka_Plugin_new(GWEN_PLUGIN_MANAGER *pm,
                                                 const char *modName,
                                                 const char *fileName);

static int GWENHYWFAR_CB
  LC_Crypt_TokenZka_GenerateKey(GWEN_CRYPT_TOKEN *ct,
				    uint32_t kid,
				    const GWEN_CRYPT_CRYPTALGO *a,
				    uint32_t gid);


static int LC_Crypt_TokenZka__ReadNotePad(GWEN_CRYPT_TOKEN *ct, GWEN_DB_NODE *dbNotePad, uint32_t guiid);
static int LC_Crypt_TokenZka__ReadContextList(GWEN_CRYPT_TOKEN *ct, uint32_t guiid);

static GWEN_CRYPT_TOKEN_KEYINFO *LC_Crypt_TokenZka__FindKeyInfo(GWEN_CRYPT_TOKEN *ct, uint32_t id);
static int LC_Crypt_TokenZka__AddKeyInfo(GWEN_CRYPT_TOKEN *ct, GWEN_CRYPT_TOKEN_KEYINFO *ki);


#endif

