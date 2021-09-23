/***************************************************************************
    begin       : Thu Jan 09 2020
    copyright   : (C) 2020 by Herbert Ellebruch
    email       :

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifndef CHIPCARD_CARD_CHIPTANUSB_P_H
#define CHIPCARD_CARD_CHIPTANUSB_P_H

#include "chiptanusb.h"
#include "libchipcard/base/pininfo.h"
#include "libchipcard/base/card_imp.h"


typedef struct LC_CHIPTANCARD LC_CHIPTANCARD;
struct LC_CHIPTANCARD {
  LC_CARD_OPEN_FN openFn;
  LC_CARD_CLOSE_FN closeFn;

};

static void GWENHYWFAR_CB LC_ChiptanusbCard_freeData(void *bp, void *p);
static int CHIPCARD_CB LC_ChiptanusbCard_Open(LC_CARD *card);
static int CHIPCARD_CB LC_ChiptanusbCard_Close(LC_CARD *card);

#endif
