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



#ifndef CHIPCARD_SERVER2_CMD_CARD_L_H
#define CHIPCARD_SERVER2_CMD_CARD_L_H

#include <chipcard2/chipcard2.h>
#include <chipcard2-server/common/card.h>

#include <gwenhywfar/xml.h>

#include <time.h>


void LCCMD_Card_extend(LCCO_CARD *cd);
void LCCMD_Card_unextend(LCCO_CARD *cd);

GWEN_XMLNODE *LCCMD_Card_GetCardNode(const LCCO_CARD *cd);

/** does @b not take over ownership of the XML node */
void LCCMD_Card_SetCardNode(LCCO_CARD *cd, GWEN_XMLNODE *n);


#endif /* CHIPCARD_SERVER2_CMD_CARD_L_H */

