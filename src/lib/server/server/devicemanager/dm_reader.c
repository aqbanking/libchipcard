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


#include "dm_reader_p.h"
#include "common/driverinfo.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


static GWEN_TYPE_UINT32 LCDM_Reader_LastId=0;

GWEN_INHERIT(LCCO_READER, LCDM_READER)



LCCO_READER *LCDM_Reader_new(LCDM_DRIVER *d, int slots){
  LCCO_READER *r;
  LCDM_READER *xr;
  int i;

  assert(d);
  assert(slots);

  r=LCCO_Reader_new();
  GWEN_NEW_OBJECT(LCDM_READER, xr);
  GWEN_INHERIT_SETDATA(LCCO_READER, LCDM_READER, r, xr,
                       LCDM_Reader_FreeData);
  xr->driver=d;
  xr->idleSince=time(0);
  LCCO_Reader_SetSlots(r, slots);

  /* assign unique id */
  if (LCDM_Reader_LastId==0)
    LCDM_Reader_LastId=time(0);
  LCCO_Reader_SetReaderId(r, ++LCDM_Reader_LastId);

  xr->slotList=LCDM_Slot_List_new();
  for (i=0; i<slots; i++)
    LCDM_Slot_List_Add(LCDM_Slot_new(), xr->slotList);

  return r;
}



LCCO_READER *LCDM_Reader_fromDb(LCDM_DRIVER *d, GWEN_DB_NODE *db){
  LCCO_READER *r;
  LCDM_READER *xr;
  int i;
  unsigned int slots;

  assert(d);

  r=LCCO_Reader_fromDb(db);
  GWEN_NEW_OBJECT(LCDM_READER, xr);
  GWEN_INHERIT_SETDATA(LCCO_READER, LCDM_READER, r, xr,
                       LCDM_Reader_FreeData);
  xr->driver=d;
  xr->slotList=LCDM_Slot_List_new();

  /* assign unique id */
  if (LCDM_Reader_LastId==0)
    LCDM_Reader_LastId=time(0);
  LCCO_Reader_SetReaderId(r, ++LCDM_Reader_LastId);

  slots=LCCO_Reader_GetSlots(r);
  for (i=0; i<slots; i++)
    LCDM_Slot_List_Add(LCDM_Slot_new(), xr->slotList);

  return r;
}



void GWENHYWFAR_CB LCDM_Reader_FreeData(void *bp, void *p) {
  LCDM_READER *xr;

  xr=(LCDM_READER*)p;
  LCDM_Slot_List_free(xr->slotList);
  GWEN_FREE_OBJECT(xr);
}



GWEN_TYPE_UINT32 LCDM_Reader_GetUsageCount(const LCCO_READER *r){
  LCDM_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCDM_READER, r);
  assert(xr);
  return xr->usageCount;
}



void LCDM_Reader_IncUsageCount(LCCO_READER *r, int count){
  LCDM_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCDM_READER, r);
  assert(xr);
  assert(count);
  xr->usageCount+=count;

  xr->idleSince=(time_t)0;
  DBG_VERBOUS(0, "Incremented Usage count of reader \"%s\" to %d",
              LCCO_Reader_GetReaderName(r), xr->usageCount);
}



void LCDM_Reader_DecUsageCount(LCCO_READER *r, int count){
  LCDM_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCDM_READER, r);
  assert(xr);
  assert(count);
  assert(xr->usageCount);
  assert(xr->usageCount>=count);
  if ((xr->usageCount-=count)==0) {
    xr->idleSince=time(0);
    DBG_VERBOUS(0, "Reader \"%s\" became idle",
                LCCO_Reader_GetReaderName(r));
  }
}



time_t LCDM_Reader_GetIdleSince(const LCCO_READER *r){
  LCDM_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCDM_READER, r);
  assert(xr);
  return xr->idleSince;
}



LCDM_DRIVER *LCDM_Reader_GetDriver(const LCCO_READER *r){
  LCDM_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCDM_READER, r);
  assert(xr);
  return xr->driver;
}



void LCDM_Reader_SetTimeout(LCCO_READER *r, int secs) {
  LCDM_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCDM_READER, r);
  assert(xr);
  if (secs==0)
    xr->timeout=0;
  else {
    time_t t;
    assert(xr);

    t=time(0);
    t+=secs;
    xr->timeout=t;
  }
}



int LCDM_Reader_CheckTimeout(const LCCO_READER *r) {
  LCDM_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCDM_READER, r);
  assert(xr);
  if (xr->timeout==0)
    return -1;
  else {
    time_t t;

    assert(xr);
    t=time(0);
    return (difftime(t, xr->timeout)>0);
  }
}



GWEN_TYPE_UINT32 LCDM_Reader_GetCurrentRequestId(const LCCO_READER *r) {
  LCDM_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCDM_READER, r);
  assert(xr);
  return xr->currentRequestId;
}



void LCDM_Reader_SetCurrentRequestId(LCCO_READER *r, GWEN_TYPE_UINT32 rid) {
  LCDM_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCDM_READER, r);
  assert(xr);
  xr->currentRequestId=rid;
}



LCS_LOCKMANAGER *LCDM_Reader_GetLockManager(const LCCO_READER *r, int slot) {
  LCDM_SLOT *sl;
  LCDM_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCDM_READER, r);
  assert(xr);

  sl=LCDM_Slot_List_First(xr->slotList);
  while(sl && slot--)
    sl=LCDM_Slot_List_Next(sl);
  if (sl)
    return LCDM_Slot_GetLockManager(sl);
  return 0;
}



GWEN_TYPE_UINT32 LCDM_Reader_LockReader(LCCO_READER *r,
                                        GWEN_TYPE_UINT32 clid,
                                        int maxLockTime,
                                        int maxLockCount) {
  GWEN_TYPE_UINT32 rqid;
  LCDM_SLOT *sl;
  LCDM_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCDM_READER, r);
  assert(xr);

  rqid=LCS_LockManager_GetNextRequestId();
  assert(rqid);

  sl=LCDM_Slot_List_First(xr->slotList);
  while(sl) {
    LCS_LOCKMANAGER *lm;
    int rv;

    lm=LCDM_Slot_GetLockManager(sl);
    assert(lm);
    rv=LCS_LockManager_RequestLockWithId(lm, rqid, clid,
                                         maxLockTime,
                                         maxLockCount);
    assert(rv==0);
    sl=LCDM_Slot_List_Next(sl);
  }

  return rqid;
}



int LCDM_Reader_CheckLockRequest(LCCO_READER *r,
                                 GWEN_TYPE_UINT32 rqid) {
  LCDM_SLOT *sl;
  LCDM_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCDM_READER, r);
  assert(xr);

  sl=LCDM_Slot_List_First(xr->slotList);
  while(sl) {
    LCS_LOCKMANAGER *lm;
    int rv;

    lm=LCDM_Slot_GetLockManager(sl);
    assert(lm);
    rv=LCS_LockManager_CheckAccess(lm, rqid);
    if (rv!=0) {
      DBG_NOTICE(0, "CheckAccess not approved, checking request (%d)", rv);
      rv=LCS_LockManager_CheckRequest(lm, rqid);
      if (rv) {
        DBG_NOTICE(0, "CheckRequest not approved (%d)", rv);
        return 1;
      }
    }
    sl=LCDM_Slot_List_Next(sl);
  }

  /* all requests succeeded */
  return 0;
}



int LCDM_Reader_RemoveLockRequest(LCCO_READER *r,
                                  GWEN_TYPE_UINT32 rqid) {
  LCDM_SLOT *sl;
  LCDM_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCDM_READER, r);
  assert(xr);

  assert(rqid);

  sl=LCDM_Slot_List_First(xr->slotList);
  while(sl) {
    LCS_LOCKMANAGER *lm;

    lm=LCDM_Slot_GetLockManager(sl);
    assert(lm);
    if (LCS_LockManager_RemoveRequest(lm, rqid)<0)
      LCS_LockManager_Unlock(lm, rqid);
    sl=LCDM_Slot_List_Next(sl);
  }

  /* all requests removed */
  return 0;
}



int LCDM_Reader_CheckLockAccess(LCCO_READER *r,
                                GWEN_TYPE_UINT32 rqid) {
  LCDM_SLOT *sl;
  LCDM_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCDM_READER, r);
  assert(xr);

  sl=LCDM_Slot_List_First(xr->slotList);
  while(sl) {
    LCS_LOCKMANAGER *lm;
    int rv;

    lm=LCDM_Slot_GetLockManager(sl);
    assert(lm);
    rv=LCS_LockManager_CheckAccess(lm, rqid);
    if (rv) {
      DBG_NOTICE(0, "CheckRequest not approved (%d)", rv);
      return rv;
    }
    sl=LCDM_Slot_List_Next(sl);
  }

  /* all requests succeeded */
  return 0;
}



int LCDM_Reader_Unlock(LCCO_READER *r, GWEN_TYPE_UINT32 rqid) {
  LCDM_SLOT *sl;
  LCDM_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCDM_READER, r);
  assert(xr);

  assert(rqid);

  sl=LCDM_Slot_List_First(xr->slotList);
  while(sl) {
    LCS_LOCKMANAGER *lm;

    lm=LCDM_Slot_GetLockManager(sl);
    assert(lm);
    LCS_LockManager_Unlock(lm, rqid);
    sl=LCDM_Slot_List_Next(sl);
  }

  /* all slot unlocked */
  return 0;
}







