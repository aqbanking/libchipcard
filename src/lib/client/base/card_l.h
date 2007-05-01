/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: card.h 163 2006-02-15 19:31:45Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CLIENT_CARD_L_H
#define CHIPCARD_CLIENT_CARD_L_H


#include "card_imp.h"


GWEN_XMLNODE *LC_Card_FindCommand(LC_CARD *card,
                                  const char *commandName);

int LC_Card_IsConnected(const LC_CARD *card);

void LC_Card_SetConnected(LC_CARD *card, int b);

#endif /* CHIPCARD_CLIENT_CARD_L_H */
