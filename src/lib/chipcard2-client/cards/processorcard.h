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


#ifndef CHIPCARD_CARD_PROCESSORCARD_H
#define CHIPCARD_CARD_PROCESSORCARD_H

#include <chipcard2-client/client/card.h>

#ifdef __cplusplus
extern "C" {
#endif

CHIPCARD_API
int LC_ProcessorCard_ExtendCard(LC_CARD *card);
CHIPCARD_API
int LC_ProcessorCard_UnextendCard(LC_CARD *card);

CHIPCARD_API
LC_CLIENT_RESULT LC_ProcessorCard_SelectDF(LC_CARD *card,
                                           const char *fname);
CHIPCARD_API
LC_CLIENT_RESULT LC_ProcessorCard_SelectEF(LC_CARD *card,
                                           const char *fname);
CHIPCARD_API CHIPCARD_DEPRECATED
LC_CLIENT_RESULT LC_ProcessorCard_ReadRecord(LC_CARD *card,
                                             int recNum,
                                             GWEN_BUFFER *buf);
CHIPCARD_API CHIPCARD_DEPRECATED
LC_CLIENT_RESULT LC_ProcessorCard_WriteRecord(LC_CARD *card,
                                              int recNum,
                                              GWEN_BUFFER *buf);


#ifdef __cplusplus
}
#endif

#endif /* CHIPCARD_CARD_PROCESSORCARD_H */


