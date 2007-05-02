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



#ifndef CHIPCARD_SERVER_COMMON_CARD_H
#define CHIPCARD_SERVER_COMMON_CARD_H


#include <gwenhywfar/buffer.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/list2.h>
#include <gwenhywfar/stringlist.h>

#include <time.h>
#include <stdio.h>


typedef struct LCCO_CARD LCCO_CARD;


GWEN_LIST_FUNCTION_DEFS(LCCO_CARD, LCCO_Card)
GWEN_LIST2_FUNCTION_DEFS(LCCO_CARD, LCCO_Card)
GWEN_INHERIT_FUNCTION_DEFS(LCCO_CARD)


#include <chipcard/chipcard.h>


LCCO_CARD *LCCO_Card_new();
void LCCO_Card_Attach(LCCO_CARD *cd);
void LCCO_Card_free(LCCO_CARD *cd);

void LCCO_Card_List2_freeAll(LCCO_CARD_LIST2 *cl);


GWEN_TYPE_UINT32 LCCO_Card_GetCardId(const LCCO_CARD *cd);
void LCCO_Card_SetCardId(LCCO_CARD *cd, GWEN_TYPE_UINT32 id);

LC_CARD_TYPE LCCO_Card_GetCardType(const LCCO_CARD *cd);
void LCCO_Card_SetCardType(LCCO_CARD *cd, LC_CARD_TYPE ct);

GWEN_TYPE_UINT32 LCCO_Card_GetReaderId(const LCCO_CARD *cd);
void LCCO_Card_SetReaderId(LCCO_CARD *cd, GWEN_TYPE_UINT32 id);

GWEN_TYPE_UINT32 LCCO_Card_GetDriversReaderId(const LCCO_CARD *cd);
void LCCO_Card_SetDriversReaderId(LCCO_CARD *cd, GWEN_TYPE_UINT32 id);

int LCCO_Card_GetSlotNum(const LCCO_CARD *cd);
void LCCO_Card_SetSlotNum(LCCO_CARD *cd, int i);

GWEN_TYPE_UINT32 LCCO_Card_GetCardNum(const LCCO_CARD *cd);
void LCCO_Card_SetCardNum(LCCO_CARD *cd, GWEN_TYPE_UINT32 i);

GWEN_TYPE_UINT32 LCCO_Card_GetReadersCardId(const LCCO_CARD *cd);
void LCCO_Card_SetReadersCardId(LCCO_CARD *cd, GWEN_TYPE_UINT32 id);

GWEN_TYPE_UINT32 LCCO_Card_GetReaderFlags(const LCCO_CARD *cd);
void LCCO_Card_SetReaderFlags(LCCO_CARD *cd, GWEN_TYPE_UINT32 fl);
void LCCO_Card_AddReaderFlags(LCCO_CARD *cd, GWEN_TYPE_UINT32 fl);
void LCCO_Card_SubReaderFlags(LCCO_CARD *cd, GWEN_TYPE_UINT32 fl);

LC_CARD_STATUS LCCO_Card_GetStatus(const LCCO_CARD *cd);
void LCCO_Card_SetStatus(LCCO_CARD *cd, LC_CARD_STATUS st);

const char *LCCO_Card_GetAtr(const LCCO_CARD *cd, unsigned int *len);
void LCCO_Card_SetAtr(LCCO_CARD *cd,
                      const char *s, unsigned int len);

const GWEN_STRINGLIST *LCCO_Card_GetTypes(const LCCO_CARD *cd);
int LCCO_Card_AddType(LCCO_CARD *cd, const char *s);

const char *LCCO_Card_GetDriverTypeName(const LCCO_CARD *cd);
void LCCO_Card_SetDriverTypeName(LCCO_CARD *cd, const char *s);

const char *LCCO_Card_GetReaderTypeName(const LCCO_CARD *cd);
void LCCO_Card_SetReaderTypeName(LCCO_CARD *cd, const char *s);

GWEN_TYPE_UINT32 LCCO_Card_GetLockId(const LCCO_CARD *cd);
void LCCO_Card_SetLockId(LCCO_CARD *cd, GWEN_TYPE_UINT32 lid);


void LCCO_Card_Dump(const LCCO_CARD *cd, FILE *f, int indent);


#endif /* CHIPCARD_SERVER_COMMON_CARD_H */





