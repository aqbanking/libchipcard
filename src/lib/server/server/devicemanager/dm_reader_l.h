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


#ifndef CHIPCARD_SERVER_DM_READER_L_H
#define CHIPCARD_SERVER_DM_READER_L_H


#include <gwenhywfar/db.h>
#include <gwenhywfar/misc.h>

#include <chipcard/chipcard.h>
#include "common/reader.h"
#include "lockmanager_l.h"

#include <time.h>

#include <chipcard/chipcard.h>
#include "devicemanager_l.h"
#include "dm_driver_l.h"
#include "dm_slot_l.h"


LCCO_READER *LCDM_Reader_new(LCDM_DRIVER *d, int slots);
LCCO_READER *LCDM_Reader_fromDb(LCDM_DRIVER *d, GWEN_DB_NODE *db);

LCDM_DRIVER *LCDM_Reader_GetDriver(const LCCO_READER *r);

void LCDM_Reader_SetTimeout(LCCO_READER *r, int secs);
int LCDM_Reader_CheckTimeout(const LCCO_READER *r);

GWEN_TYPE_UINT32 LCDM_Reader_GetUsageCount(const LCCO_READER *r);
void LCDM_Reader_IncUsageCount(LCCO_READER *r, int count);
void LCDM_Reader_DecUsageCount(LCCO_READER *r, int count);
time_t LCDM_Reader_GetIdleSince(const LCCO_READER *r);

GWEN_TYPE_UINT32 LCDM_Reader_GetCurrentRequestId(const LCCO_READER *r);
void LCDM_Reader_SetCurrentRequestId(LCCO_READER *r, GWEN_TYPE_UINT32 rid);


LCS_LOCKMANAGER *LCDM_Reader_GetLockManager(const LCCO_READER *r, int slot);

GWEN_TYPE_UINT32 LCDM_Reader_LockReader(LCCO_READER *r,
                                        GWEN_TYPE_UINT32 clid,
                                        int maxLockTime,
                                        int maxLockCount);
int LCDM_Reader_CheckLockRequest(LCCO_READER *r,
                                 GWEN_TYPE_UINT32 reqid);
int LCDM_Reader_RemoveLockRequest(LCCO_READER *r,
                                  GWEN_TYPE_UINT32 rqid);

int LCDM_Reader_CheckLockAccess(LCCO_READER *r,
                                GWEN_TYPE_UINT32 rqid);

int LCDM_Reader_Unlock(LCCO_READER *r, GWEN_TYPE_UINT32 rqid);





#endif /* CHIPCARD_SERVER_DM_READER_L_H */


