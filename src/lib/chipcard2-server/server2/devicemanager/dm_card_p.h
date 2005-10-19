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



#ifndef CHIPCARD_SERVER_DM_CARD_P_H
#define CHIPCARD_SERVER_DM_CARD_P_H

#include "dm_card_l.h"


typedef struct LCDM_CARD LCDM_CARD;
struct LCDM_CARD {
  LCDM_READER *reader;
};


void LCDM_Card_FreeData(void *bp, void *p);




#endif /* CHIPCARD_SERVER_DM_CARD_H */

