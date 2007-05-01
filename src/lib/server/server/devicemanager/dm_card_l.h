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



#ifndef CHIPCARD_SERVER_DM_CARD_L_H
#define CHIPCARD_SERVER_DM_CARD_L_H

#include "dm_reader_l.h"
#include "common/card.h"

void LCDM_Card_extend(LCCO_CARD *cd,
                      LCCO_READER *r);
void LCDM_Card_unextend(LCCO_CARD *cd);


LCCO_READER *LCDM_Card_GetReader(const LCCO_CARD *cd);


#endif /* CHIPCARD_SERVER_DM_CARD_L_H */

