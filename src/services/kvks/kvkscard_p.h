/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id$
    begin       : Sun Jun 13 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

/** @file examplecard_p.h
 *  @short Private header file only to be used by examplecard.c
 *
 * This file is not to be included by any other file except the corresponding
 * source file (in this case examplecard.c). It only contains definitions
 * private to this type. This allows to follow the object oriented programming
 * paradigm used by Libchipcard2.
 */

#ifndef CHIPCARD_CARD_KVKSCARD_P_H
#define CHIPCARD_CARD_KVKSCARD_P_H

#include "kvkscard.h"

/**
 * This type definition MUST NEVER be used outside this file and the
 * corresponding source file (examplecard.c) !
 * This makes this type invisible to the outside, which is very much intended.
 * This way a class may be changed later without corrupting the API.
 */
typedef struct KVKS_CARD KVKS_CARD;

struct KVKS_CARD {
  LC_CARD_OPEN_FN openFn;
  LC_CARD_CLOSE_FN closeFn;

  GWEN_BUFFER *dataBuffer;
  KVKS_STATUS status;
  uint32_t currentRequest;

  GWEN_DB_NODE *dbCardData;
  int checkSumOk;
};


void GWENHYWFAR_CB KVKSCard_freeData(void *bp, void *p);


LC_CLIENT_RESULT KVKSCard_Open(LC_CARD *card);
LC_CLIENT_RESULT KVKSCard_Close(LC_CARD *card);




#endif /* CHIPCARD_CARD_KVKSCARD_P_H */




