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



#ifndef CHIPCARD_SERVER2_CMD_CARD_P_H
#define CHIPCARD_SERVER2_CMD_CARD_P_H

#include "cmd_card_l.h"


typedef struct LCCMD_CARD LCCMD_CARD;
struct LCCMD_CARD {
  GWEN_XMLNODE *cardNode;
};


void LCCMD_Card_FreeData(void *bp, void *p);


#endif /* CHIPCARD_SERVER2_CMD_CARD_P_H */

