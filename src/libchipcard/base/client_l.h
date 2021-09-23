/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004-2010 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CLIENT_CLIENT_L_H
#define CHIPCARD_CLIENT_CLIENT_L_H

#include "libchipcard/base/client.h"
#include "libchipcard/base/card.h"

#include <gwenhywfar/msgengine.h>


int LC_Client_ExecApdu(LC_CLIENT *cl,
                       LC_CARD *card,
                       const char *apdu,
                       unsigned int len,
                       GWEN_BUFFER *rbuf,
                       LC_CLIENT_CMDTARGET t);

GWEN_MSGENGINE *LC_Client_GetMsgEngine(const LC_CLIENT *cl);


#endif /* CHIPCARD_CLIENT_CLIENT_L_H */



