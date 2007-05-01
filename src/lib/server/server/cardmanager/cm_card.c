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


#include "cm_card_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_INHERIT(LCCO_CARD, LCCM_CARD)

static GWEN_TYPE_UINT32 lccm_card__next_request_id=0;


void LCCM_Card_extend(LCCO_CARD *cd) {
  LCCM_CARD *dc;

  assert(cd);

  GWEN_NEW_OBJECT(LCCM_CARD, dc);
  dc->requestList=LCS_LockRequest_List_new();
  dc->lastStatus=LCCO_Card_GetStatus(cd);
  dc->unusedSince=time(0);
  GWEN_INHERIT_SETDATA(LCCO_CARD, LCCM_CARD, cd, dc,
                       LCCM_Card_FreeData);
}



void LCCM_Card_unextend(LCCO_CARD *cd) {
  LCCM_CARD *dc;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCM_CARD, cd);
  assert(dc);

  GWEN_INHERIT_UNLINK(LCCO_CARD, LCCM_CARD, cd);
  LCCM_Card_FreeData(cd, dc);
}



void GWENHYWFAR_CB LCCM_Card_FreeData(void *bp, void *p) {
  LCCM_CARD *dc;

  dc=(LCCM_CARD*)p;
  LCS_LockRequest_free(dc->currentRequest);
  LCS_LockRequest_List_free(dc->requestList);
  GWEN_FREE_OBJECT(p);
}



int LCCM_Card_RequestLock(LCCO_CARD *cd,
                          GWEN_TYPE_UINT32 clid,
                          int duration,
                          int maxLocks) {
  LCCM_CARD *dc;
  LCS_LOCKREQUEST *rq;
  int count;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCM_CARD, cd);
  assert(dc);

  if (LCCO_Card_GetStatus(cd)!=LC_CardStatusInserted) {
    DBG_ERROR(0, "Card is not inserted");
    return -LC_ERROR_CARD_REMOVED;
  }

  if (lccm_card__next_request_id==0)
    lccm_card__next_request_id=time(0);

  count=LCCM_Card_CountClientRequests(cd, clid);
  if ((count+1)>=maxLocks) {
    DBG_WARN(0, "Maximum number of locks reached (%d)", count);
    return -LC_ERROR_INVALID;
  }

  rq=LCS_LockRequest_new();
  LCS_LockRequest_SetRequestId(rq, lccm_card__next_request_id++);
  LCS_LockRequest_SetClientId(rq, clid);
  LCS_LockRequest_SetDuration(rq, duration);

  LCS_LockRequest_List_Add(rq, dc->requestList);

  dc->unusedSince=0;

  return 0;
}



LCS_LOCKREQUEST *LCCM_Card_FindRequestByClientId(LCCO_CARD *cd,
                                                  GWEN_TYPE_UINT32 clid){
  LCCM_CARD *dc;
  LCS_LOCKREQUEST *rq;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCM_CARD, cd);
  assert(dc);

  rq=dc->currentRequest;
  if (rq && clid==LCS_LockRequest_GetClientId(rq))
    return rq;

  rq=LCS_LockRequest_List_First(dc->requestList);
  while(rq) {
    if (clid==LCS_LockRequest_GetRequestId(rq))
      break;
    rq=LCS_LockRequest_List_Next(rq);
  }

  return rq;
}



int LCCM_Card_CountClientRequests(const LCCO_CARD *cd,
                                  GWEN_TYPE_UINT32 clid){
  LCCM_CARD *dc;
  LCS_LOCKREQUEST *rq;
  int count=0;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCM_CARD, cd);
  assert(dc);

  rq=dc->currentRequest;
  if (rq && clid==LCS_LockRequest_GetClientId(rq))
    count++;

  rq=LCS_LockRequest_List_First(dc->requestList);
  while(rq) {
    if (clid==LCS_LockRequest_GetRequestId(rq))
      count++;
    rq=LCS_LockRequest_List_Next(rq);
  }

  return count;
}



int LCCM_Card_CheckRequest(LCCO_CARD *cd, GWEN_TYPE_UINT32 clid) {
  LCCM_CARD *dc;
  LCS_LOCKREQUEST *rq;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCM_CARD, cd);
  assert(dc);

  if (LCCO_Card_GetStatus(cd)!=LC_CardStatusInserted) {
    DBG_ERROR(0, "Card is not inserted");
    return -LC_ERROR_CARD_REMOVED;
  }

  rq=LCS_LockRequest_List_First(dc->requestList);
  assert(rq);

  if (LCS_LockRequest_GetClientId(rq)==clid &&
      dc->currentRequest==0) {
    GWEN_TIME *ti;

    DBG_NOTICE(0, "Lock request granted");
    LCS_LockRequest_List_Del(rq);
    dc->currentRequest=rq;

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



int LCCM_Card_RemoveRequest(LCCO_CARD *cd, GWEN_TYPE_UINT32 clid) {
  LCCM_CARD *dc;
  LCS_LOCKREQUEST *rq;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCM_CARD, cd);
  assert(dc);

  rq=LCCM_Card_FindRequestByClientId(cd, clid);
  assert(rq);

  /* must not be the currently active lock */
  assert(rq!=dc->currentRequest);
  LCS_LockRequest_List_Del(rq);

  if (LCS_LockRequest_List_GetCount(dc->requestList)==0 &&
      dc->currentRequest==0 &&
      dc->unusedSince==0)
    dc->unusedSince=time(0);

  return 0;
}



void LCCM_Card_RemoveAllClientRequests(LCCO_CARD *cd,
                                       GWEN_TYPE_UINT32 clid){
  LCCM_CARD *dc;
  LCS_LOCKREQUEST *rq;
  int prevCount;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCM_CARD, cd);
  assert(dc);

  prevCount=LCS_LockRequest_List_GetCount(dc->requestList);
  if (dc->currentRequest)
    prevCount++;

  if (prevCount==0)
    return;

  if (dc->currentRequest &&
      clid==LCS_LockRequest_GetClientId(dc->currentRequest)) {
    LCS_LockRequest_free(dc->currentRequest);
    dc->currentRequest=0;
  }

  rq=LCS_LockRequest_List_First(dc->requestList);
  while(rq) {
    LCS_LOCKREQUEST *next;

    next=LCS_LockRequest_List_Next(rq);
    if (clid==LCS_LockRequest_GetClientId(rq)) {
      LCS_LockRequest_List_Del(rq);
      LCS_LockRequest_free(rq);
    }

    rq=next;
  }

  if (LCS_LockRequest_List_GetCount(dc->requestList)==0 &&
      dc->currentRequest==0 &&
      dc->unusedSince==0)
    dc->unusedSince=time(0);
}



int LCCM_Card_Unlock(LCCO_CARD *cd, GWEN_TYPE_UINT32 clid) {
  LCCM_CARD *dc;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCM_CARD, cd);
  assert(dc);

  if (dc->currentRequest &&
      clid==LCS_LockRequest_GetClientId(dc->currentRequest)) {
    DBG_NOTICE(0, "Unlocking card from client %08x", clid);
    LCS_LockRequest_free(dc->currentRequest);
    dc->currentRequest=0;
    if (LCS_LockRequest_List_GetCount(dc->requestList)==0 &&
        dc->unusedSince==0)
      dc->unusedSince=time(0);
    return 0;
  }

  return -LC_ERROR_CARD_NOT_OWNED;
}



int LCCM_Card_HasLockRequests(const LCCO_CARD *cd) {
  LCCM_CARD *dc;
  int count=0;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCM_CARD, cd);
  assert(dc);

  if (dc->currentRequest)
    count++;
  count+=LCS_LockRequest_List_GetCount(dc->requestList);

  if (count)
    return 1;
  return 0;
}



int LCCM_Card_CheckAccess(LCCO_CARD *cd, GWEN_TYPE_UINT32 clid) {
  LCCM_CARD *dc;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCM_CARD, cd);
  assert(dc);

  if (dc->currentRequest &&
      clid==LCS_LockRequest_GetClientId(dc->currentRequest)) {
    /* ok, locked by this client */
    return 0;
  }
  DBG_DEBUG(0, "Card not locked by this client");

  return LC_ERROR_NOT_LOCKED;
}



time_t LCCM_Card_GetUnusedSince(const LCCO_CARD *cd) {
  LCCM_CARD *dc;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCM_CARD, cd);
  assert(dc);

  return dc->unusedSince;
}



LC_CARD_STATUS LCCM_Card_GetLastStatus(const LCCO_CARD *cd) {
  LCCM_CARD *dc;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCM_CARD, cd);
  assert(dc);

  return dc->lastStatus;
}



void LCCM_Card_SetLastStatus(LCCO_CARD *cd, LC_CARD_STATUS st) {
  LCCM_CARD *dc;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCM_CARD, cd);
  assert(dc);

  if (dc->lastStatus!=st && st==LC_CardStatusRemoved &&
      dc->unusedSince==0)
    dc->unusedSince=time(0);

  dc->lastStatus=st;

}



int LCCM_Card_GetReaderIsInUse(const LCCO_CARD *cd) {
  LCCM_CARD *dc;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCM_CARD, cd);
  assert(dc);

  return dc->readerIsInUse;
}



void LCCM_Card_SetReaderIsInUse(LCCO_CARD *cd, int i) {
  LCCM_CARD *dc;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCM_CARD, cd);
  assert(dc);

  dc->readerIsInUse=i;
}



time_t LCCM_Card_GetLastAdTime(const LCCO_CARD *cd) {
  LCCM_CARD *dc;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCM_CARD, cd);
  assert(dc);

  return dc->lastAdTime;
}



void LCCM_Card_SetLastAdTime(LCCO_CARD *cd, time_t t) {
  LCCM_CARD *dc;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCM_CARD, cd);
  assert(dc);

  dc->lastAdTime=t;
}







