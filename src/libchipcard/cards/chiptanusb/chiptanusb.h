/***************************************************************************
    begin       : Thu Jan 09 2020
    copyright   : (C) 2020 by Herbert Ellebruch
    email       :

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CARD_CHIPTANUSB_H
#define CHIPCARD_CARD_CHIPTANUSB_H

#include <libchipcard/base/card.h>
#include <libchipcard/base/pininfo.h>
#include <gwenhywfar/db.h>


CHIPCARD_API
int LC_ChiptanusbCard_ExtendCard(LC_CARD *card);
CHIPCARD_API
int LC_ChiptanusbCard_UnextendCard(LC_CARD *card);
CHIPCARD_API
LC_CLIENT_RESULT LC_ChiptanusbCard_Reopen(LC_CARD *card);

/** @name General Card Data
 *
 */
CHIPCARD_API
LC_CLIENT_RESULT LC_ChiptanusbCard_GenerateTan(LC_CARD *card,
                                               unsigned char *pCommand, int CommandSize, GWEN_BUFFER *buf);

#endif


