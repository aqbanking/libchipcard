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



#ifndef CHIPCARD_SERVER_CARD_L_H
#define CHIPCARD_SERVER_CARD_L_H

#include <chipcard2-server/server/card.h>
#include "commands/cardcontext_l.h"


LC_CARDCONTEXT *LC_Card_GetContext(const LC_CARD *cd);

/**
 * Takes over ownership of the card context.
 */
void LC_Card_SetContext(LC_CARD *cd, LC_CARDCONTEXT *ctx);

#endif /* CHIPCARD_SERVER_CARD_L_H */





