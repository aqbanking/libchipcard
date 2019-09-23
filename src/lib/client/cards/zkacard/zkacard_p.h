/***************************************************************************
    begin       : Sat Nov 13 2010
    copyright   : (C) 2010 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CARD_ZKACARD_P_H
#define CHIPCARD_CARD_ZKACARD_P_H

#include "zkacard.h"
#include "pininfo_l.h"

#include <chipcard/card_imp.h>



typedef struct LC_ZKACARD LC_ZKACARD;
struct LC_ZKACARD {
  LC_CARD_OPEN_FN openFn;
  LC_CARD_CLOSE_FN closeFn;

  GWEN_BUFFER *bin_ef_id;
  GWEN_DB_NODE *db_ef_id;

  GWEN_BUFFER *bin_ef_gd_0;
  GWEN_BUFFER *bin_ef_ssd;
  GWEN_DB_NODE *db_ef_ssd;

  int len_modus_sk_ch_ds;
  int len_modus_sk_ch_aut;
  int len_modus_sk_ch_ke;
  int min_len_csa_password;
  int key_flag;

  LC_PININFO_LIST *pinInfoList;
};


static void GWENHYWFAR_CB LC_ZkaCard_freeData(void *bp, void *p);
static LC_CLIENT_RESULT CHIPCARD_CB LC_ZkaCard_Open(LC_CARD *card);
static LC_CLIENT_RESULT CHIPCARD_CB LC_ZkaCard_Close(LC_CARD *card);
static LC_CLIENT_RESULT CHIPCARD_CB LC_ZkaCard_GetPinStatus(LC_CARD *card,
                                                            unsigned int pid,
                                                            int *maxErrors,
                                                            int *currentErrors);


static LC_CLIENT_RESULT LC_ZkaCard__PrepareSign(LC_CARD *card, int globalKey, int keyId, int keyVersion);
static int LC_ZkaCard__ParsePseudoOids(const uint8_t *p, uint32_t bs, GWEN_BUFFER *mbuf);


static int LC_ZkaCard__ReadPwdd(LC_CARD *card);
static LC_CLIENT_RESULT LC_ZkaCard__ParseDfSigSSD(LC_CARD *card);
static LC_CLIENT_RESULT LC_ZkaCard__SeccosSearchRecord(LC_CARD *card,
                                                       uint32_t flags,
                                                       int recNum,
                                                       const char *searchPattern,
                                                       unsigned int searchPatternSize,
                                                       GWEN_BUFFER *buf);
#endif

