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


#include "lockmanager_p.h"
#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>



static uint32_t lcs_lockmanager__next_request_id=0;



LCS_LOCKMANAGER *LCS_LockManager_new(const char *what) {
  LCS_LOCKMANAGER *lm;

  GWEN_NEW_OBJECT(LCS_LOCKMANAGER, lm);
  lm->requestList=LCS_LockRequest_List_new();
  lm->what=strdup(what);
  return lm;
}



void LCS_LockManager_free(LCS_LOCKMANAGER *lm) {
  if (lm) {
    free(lm->what);
    LCS_LockRequest_free(lm->currentRequest);
    LCS_LockRequest_List_free(lm->requestList);
    GWEN_FREE_OBJECT(lm);
  }
}



const char *LCS_LockManager_GetObjectTypeName(const LCS_LOCKMANAGER *lm) {
  assert(lm);
  return lm->what;
}



uint32_t LCS_LockManager_GetNextRequestId() {
  if (lcs_lockmanager__next_request_id==0)
    lcs_lockmanager__next_request_id=time(0);
  return lcs_lockmanager__next_request_id++;
}



int LCS_LockManager_RequestLockWithId(LCS_LOCKMANAGER *lm,
                                      uint32_t lockid,
                                      uint32_t clid,
                                      int duration,
                                      int maxLocks) {
  LCS_LOCKREQUEST *rq;
  int count;

  assert(lm);

  count=LCS_LockManager_CountClientRequests(lm, clid);
  if ((count+1)>=maxLocks) {
    DBG_WARN(0, "Maximum number of locks reached (%d)", count);
    return -1;
  }

  rq=LCS_LockRequest_new();
  LCS_LockRequest_SetRequestId(rq, lockid);
  LCS_LockRequest_SetClientId(rq, clid);
  LCS_LockRequest_SetDuration(rq, duration);

  LCS_LockRequest_List_Add(rq, lm->requestList);

  return 0;
}



uint32_t LCS_LockManager_RequestLock(LCS_LOCKMANAGER *lm,
                                             uint32_t clid,
                                             int duration,
                                             int maxLocks) {
  int rv;
  uint32_t rqid;

  rqid=LCS_LockManager_GetNextRequestId();
  rv=LCS_LockManager_RequestLockWithId(lm, rqid, clid, duration, maxLocks);
  if (rv==0)
    return rqid;

  return 0;
}



LCS_LOCKREQUEST*
LCS_LockManager_FindRequestByRequestId(LCS_LOCKMANAGER *lm,
                                       uint32_t rqid){
  LCS_LOCKREQUEST *rq;

  assert(lm);
  rq=lm->currentRequest;
  if (rq && rqid==LCS_LockRequest_GetRequestId(rq))
    return rq;

  rq=LCS_LockRequest_List_First(lm->requestList);
  while(rq) {
    if (rqid==LCS_LockRequest_GetRequestId(rq))
      break;
    rq=LCS_LockRequest_List_Next(rq);
  }

  return rq;
}



int LCS_LockManager_CountClientRequests(const LCS_LOCKMANAGER *lm,
                                        uint32_t clid){
  LCS_LOCKREQUEST *rq;
  int count=0;

  assert(lm);

  rq=lm->currentRequest;
  if (rq && clid==LCS_LockRequest_GetClientId(rq))
    count++;

  rq=LCS_LockRequest_List_First(lm->requestList);
  while(rq) {
    if (clid==LCS_LockRequest_GetRequestId(rq))
      count++;
    rq=LCS_LockRequest_List_Next(rq);
  }

  return count;
}



int LCS_LockManager_CheckRequest(LCS_LOCKMANAGER *lm, uint32_t rqid){
  LCS_LOCKREQUEST *rq;

  assert(lm);

  rq=LCS_LockRequest_List_First(lm->requestList);
  assert(rq);

  if (LCS_LockRequest_GetRequestId(rq)==rqid &&
      lm->currentRequest==0) {
    GWEN_TIME *ti;

    DBG_NOTICE(0, "Lock request granted");
    LCS_LockRequest_List_Del(rq);
    lm->currentRequest=rq;

    /* store locking time */
    ti=GWEN_CurrentTime();
    LCS_LockRequest_SetLockTime(rq, ti);
    GWEN_Time_AddSeconds(ti, LCS_LockRequest_GetDuration(rq));
    LCS_LockRequest_SetLockUntil(rq, ti);
    GWEN_Time_free(ti);

    return 0;
  }

  return 1;
}



int LCS_LockManager_RemoveRequest(LCS_LOCKMANAGER *lm, uint32_t rqid){
  LCS_LOCKREQUEST *rq;

  assert(lm);

  rq=LCS_LockManager_FindRequestByRequestId(lm, rqid);
  assert(rq);

  /* must not be the currently active lock */
  if (rq==lm->currentRequest) {
    DBG_DEBUG(0, "Cannot remove current request");
    return -1;
  }
  LCS_LockRequest_List_Del(rq);

  return 0;
}



void LCS_LockManager_RemoveAllClientRequests(LCS_LOCKMANAGER *lm,
                                             uint32_t clid){
  LCS_LOCKREQUEST *rq;
  int prevCount;

  assert(lm);

  prevCount=LCS_LockRequest_List_GetCount(lm->requestList);
  if (lm->currentRequest)
    prevCount++;

  if (prevCount==0)
    return;

  if (lm->currentRequest &&
      clid==LCS_LockRequest_GetClientId(lm->currentRequest)) {
    DBG_NOTICE(0, "Removing lock of client \"%08x\" on %s", clid, lm->what);
    LCS_LockRequest_free(lm->currentRequest);
    lm->currentRequest=0;
  }

  rq=LCS_LockRequest_List_First(lm->requestList);
  while(rq) {
    LCS_LOCKREQUEST *next;

    next=LCS_LockRequest_List_Next(rq);
    if (clid==LCS_LockRequest_GetClientId(rq)) {
      DBG_NOTICE(0, "Removing lock request for client \"%08x\" on %s",
                 clid, lm->what);
      LCS_LockRequest_List_Del(rq);
      LCS_LockRequest_free(rq);
    }

    rq=next;
  }
}



int LCS_LockManager_Unlock(LCS_LOCKMANAGER *lm, uint32_t rqid) {
  assert(lm);

  if (lm->currentRequest &&
      rqid==LCS_LockRequest_GetRequestId(lm->currentRequest)) {
    DBG_NOTICE(0, "Unlocking %s (request %08x)", lm->what, rqid);
    LCS_LockRequest_free(lm->currentRequest);
    lm->currentRequest=0;
    return 0;
  }

  return -1;
}



int LCS_LockManager_HasLockRequests(const LCS_LOCKMANAGER *lm) {
  int count=0;

  assert(lm);

  if (lm->currentRequest)
    count++;
  count+=LCS_LockRequest_List_GetCount(lm->requestList);

  if (count)
    return 1;
  return 0;
}



int LCS_LockManager_CheckAccess(LCS_LOCKMANAGER *lm, uint32_t rqid) {
  assert(lm);

  if (lm->currentRequest) {
    DBG_DEBUG(0, "%s currently locked by \"%08x\" (wanted: %08x)",
              lm->what,
              LCS_LockRequest_GetRequestId(lm->currentRequest),
              rqid);
  }

  if ((lm->currentRequest &&
       rqid==LCS_LockRequest_GetRequestId(lm->currentRequest)) ||
      (lm->currentRequest==0 && rqid==0)){
    /* ok, locked by this client */
    return 0;
  }
  DBG_DEBUG(0, "%s not locked by this client", lm->what);

  return -1;
}




