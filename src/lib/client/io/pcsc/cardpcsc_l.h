/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: client_p.h 137 2005-11-03 13:07:50Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CLIENT_CARDPCSC_L_H
#define CHIPCARD_CLIENT_CARDPCSC_L_H

#define LC_CARD_EXTEND_CLIENT
#include <chipcard/client/card_imp.h>

#include "clientpcsc_l.h"


LC_CARD *LC_CardPcsc_new(LC_CLIENT *cl,
                         uint32_t cardId,
                         SCARDHANDLE scardHandle,
                         const char *readerName,
                         DWORD protocol,
                         const char *cardType,
                         uint32_t rflags,
                         const unsigned char *atrBuf,
                         unsigned int atrLen);


SCARDHANDLE LC_CardPcsc_GetScardHandle(const LC_CARD *card);
const char *LC_CardPcsc_GetReaderName(const LC_CARD *card);
DWORD LC_CardPcsc_GetProtocol(const LC_CARD *card);


#endif /* CHIPCARD_CLIENT_CARDPCSC_L_H */

