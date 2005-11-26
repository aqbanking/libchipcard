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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "dm_slot_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LCDM_SLOT, LCDM_Slot);


LCDM_SLOT *LCDM_Slot_new() {
  LCDM_SLOT *sl;

  GWEN_NEW_OBJECT(LCDM_SLOT, sl);
  DBG_MEM_INC("LCDM_SLOT", 0);
  GWEN_LIST_INIT(LCDM_SLOT, sl);
  sl->lockManager=LCS_LockManager_new("slot");

  return sl;
}



void LCDM_Slot_free(LCDM_SLOT *sl) {
  if (sl) {
    GWEN_LIST_FINI(LCDM_SLOT, sl);
    LCS_LockManager_free(sl->lockManager);

    DBG_MEM_DEC("LCDM_SLOT");
    GWEN_FREE_OBJECT(sl);
  }
}



LCS_LOCKMANAGER *LCDM_Slot_GetLockManager(const LCDM_SLOT *sl) {
  assert(sl);
  return sl->lockManager;
}










