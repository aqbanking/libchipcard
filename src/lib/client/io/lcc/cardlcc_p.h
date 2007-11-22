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


#ifndef CHIPCARD_CLIENT_CARDLCC_P_H
#define CHIPCARD_CLIENT_CARDLCC_P_H

#include "cardlcc_l.h"

typedef struct LC_CARD_LCC LC_CARD_LCC;
struct LC_CARD_LCC {
  uint32_t serverId;
  int connected;
};
void GWENHYWFAR_CB LC_CardLcc_FreeData(void *bp, void *p);



#endif /* CHIPCARD_CLIENT_CARDLCC_P_H */

