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


#ifndef CHIPCARD_SERVER_DM_SLOT_P_H
#define CHIPCARD_SERVER_DM_SLOT_P_H


#include "dm_slot_l.h"

#include "common/devmonitor.h"



struct LCDM_SLOT {
  GWEN_LIST_ELEMENT(LCDM_SLOT);
  LCS_LOCKMANAGER *lockManager;
};


#endif /* CHIPCARD_SERVER_DM_SLOT_P_H */


