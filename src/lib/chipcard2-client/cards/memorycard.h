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


#ifndef CHIPCARD_CARD_MEMORYCARD_H
#define CHIPCARD_CARD_MEMORYCARD_H

#include <chipcard2-client/client/card.h>


#ifdef __cplusplus
extern "C" {
#endif


CHIPCARD_API
int LC_MemoryCard_ExtendCard(LC_CARD *card);
CHIPCARD_API
int LC_MemoryCard_UnextendCard(LC_CARD *card);

CHIPCARD_API
LC_CLIENT_RESULT LC_MemoryCard_ReadBinary(LC_CARD *card,
                                          int offset,
                                          int size,
                                          GWEN_BUFFER *buf);
CHIPCARD_API
LC_CLIENT_RESULT LC_MemoryCard_WriteBinary(LC_CARD *card,
                                           int offset,
                                           const char *ptr,
                                           unsigned int size);

CHIPCARD_API
unsigned int LC_MemoryCard_GetCapacity(const LC_CARD *card);



#ifdef __cplusplus
}
#endif



#endif /* CHIPCARD_CARD_MEMORYCARD_H */


