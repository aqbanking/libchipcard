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


#ifndef CHIPCARD_SERVER_DM_READER_P_H
#define CHIPCARD_SERVER_DM_READER_P_H


#include "dm_reader_l.h"

#include "common/devmonitor.h"



typedef struct LCDM_READER LCDM_READER;
struct LCDM_READER {
  LCDM_DRIVER *driver;
  int wantRestart;

  time_t timeout;
  time_t idleSince;

  GWEN_TYPE_UINT32 currentRequestId;

  /** increment when attached to user or to ACTIVE cards (not when attached
   * to inactive cards!) */
  GWEN_TYPE_UINT32 usageCount;

  LCDM_SLOT_LIST *slotList;
};
static void GWENHYWFAR_CB LCDM_Reader_FreeData(void *bp, void *p);



#endif /* CHIPCARD_SERVER_DM_READER_P_H */


