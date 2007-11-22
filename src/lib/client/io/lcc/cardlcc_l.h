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


#ifndef CHIPCARD_CLIENT_CARDLCC_L_H
#define CHIPCARD_CLIENT_CARDLCC_L_H

#define LC_CARD_EXTEND_CLIENT
#include <chipcard/client/card_imp.h>


LC_CARD *LC_CardLcc_new(LC_CLIENT *cl,
                        uint32_t cardId,
                        uint32_t serverId,
                        const char *cardType,
                        uint32_t rflags,
                        const unsigned char *atrBuf,
                        unsigned int atrLen);


uint32_t LC_CardLcc_GetServerId(const LC_CARD *card);

int LC_CardLcc_IsConnected(const LC_CARD *card);
void LC_CardLcc_SetConnected(LC_CARD *card, int b);



#endif /* CHIPCARD_CLIENT_CARDLCC_L_H */

