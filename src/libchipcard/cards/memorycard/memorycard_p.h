/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2021 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CARD_MEMORYCARD_P_H
#define CHIPCARD_CARD_MEMORYCARD_P_H

#include <libchipcard/base/card_imp.h>
#include <libchipcard/cards/memorycard/memorycard.h>


#define LC_MEMORYCARD_DEFAULT_WRITEBOUNDARY 249

typedef struct LC_MEMORYCARD LC_MEMORYCARD;

struct LC_MEMORYCARD {
  LC_CARD_OPEN_FN openFn;
  LC_CARD_CLOSE_FN closeFn;
  int writeBoundary;
  unsigned int capacity;
};


void GWENHYWFAR_CB LC_MemoryCard_freeData(void *bp, void *p);

int CHIPCARD_CB LC_MemoryCard_Open(LC_CARD *card);
int LC_MemoryCard_Reopen(LC_CARD *card);
int CHIPCARD_CB LC_MemoryCard_Close(LC_CARD *card);


void LC_MemoryCard__CalculateCapacity(LC_CARD *card);


#endif /* CHIPCARD_CARD_MEMORYCARD_P_H */


