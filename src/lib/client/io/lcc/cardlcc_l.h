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
#include <chipcard3/client/card_imp.h>


LC_CARD *LC_CardLcc_new(LC_CLIENT *cl,
                        GWEN_TYPE_UINT32 cardId,
                        GWEN_TYPE_UINT32 serverId,
                        const char *cardType,
                        GWEN_TYPE_UINT32 rflags,
                        const unsigned char *atrBuf,
                        unsigned int atrLen);


GWEN_TYPE_UINT32 LC_CardLcc_GetServerId(const LC_CARD *card);

int LC_CardLcc_IsConnected(const LC_CARD *card);
void LC_CardLcc_SetConnected(LC_CARD *card, int b);



#endif /* CHIPCARD_CLIENT_CARDLCC_L_H */

