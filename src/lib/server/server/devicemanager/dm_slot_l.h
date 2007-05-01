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


#ifndef CHIPCARD_SERVER_DM_SLOT_L_H
#define CHIPCARD_SERVER_DM_SLOT_L_H


#include "lockmanager_l.h"
#include <gwenhywfar/misc.h>


typedef struct LCDM_SLOT LCDM_SLOT;

GWEN_LIST_FUNCTION_DEFS(LCDM_SLOT, LCDM_Slot);

LCDM_SLOT *LCDM_Slot_new();
void LCDM_Slot_free(LCDM_SLOT *sl);

LCS_LOCKMANAGER *LCDM_Slot_GetLockManager(const LCDM_SLOT *sl);


#endif /* CHIPCARD_SERVER_DM_SLOT_L_H */


