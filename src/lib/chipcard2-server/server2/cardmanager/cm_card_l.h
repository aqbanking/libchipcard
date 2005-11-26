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



#ifndef CHIPCARD_SERVER2_CM_CARD_L_H
#define CHIPCARD_SERVER2_CM_CARD_L_H

#include <chipcard2/chipcard2.h>
#include "common/card.h"
#include <time.h>


void LCCM_Card_extend(LCCO_CARD *cd);
void LCCM_Card_unextend(LCCO_CARD *cd);


/**
 * Request a lock on the card. The locking time will be at most the number
 * of seconds given in the parameter <i>duration</i>.
 * @return 0 if ok, !=0 on error
 * @param duration maximum number of seconds the lock will hold
 * @param maxLocks maximum number of locks allowed for this client
 */
int LCCM_Card_RequestLock(LCCO_CARD *cd,
                          GWEN_TYPE_UINT32 clid,
                          int duration,
                          int maxLocks);

/**
 * @return -1 on error, 0 if request granted, 1 of not
 */
int LCCM_Card_CheckRequest(LCCO_CARD *cd, GWEN_TYPE_UINT32 clid);

/**
 * Removes a lock applied by the combination of
 * @ref LCCM_Card_RequestLock and @ref LCCM_Card_CheckRequest.
 */
int LCCM_Card_Unlock(LCCO_CARD *cd, GWEN_TYPE_UINT32 clid);


/**
 * Removes a lock request from the list of waiting requests.
 * The request to be removed MUST NOT be the currently active one (use
 * @ref LCCM_Card_Unlock in this case).
 * @return 0 if ok, !=0 on error
 */
int LCCM_Card_RemoveRequest(LCCO_CARD *cd, GWEN_TYPE_UINT32 clid);


/**
 * Removes all lock requests for the given client. If the client already
 * acquired a lock on this card then it will be removed (thus unlocking the
 * card for use by other clients).
 */
void LCCM_Card_RemoveAllClientRequests(LCCO_CARD *cd,
                                       GWEN_TYPE_UINT32 clid);

int LCCM_Card_HasLockRequests(const LCCO_CARD *cd);


int LCCM_Card_CheckAccess(LCCO_CARD *cd, GWEN_TYPE_UINT32 clid);


time_t LCCM_Card_GetUnusedSince(const LCCO_CARD *cd);
LC_CARD_STATUS LCCM_Card_GetLastStatus(const LCCO_CARD *cd);
void LCCM_Card_SetLastStatus(LCCO_CARD *cd, LC_CARD_STATUS st);
int LCCM_Card_GetReaderIsInUse(const LCCO_CARD *cd);
void LCCM_Card_SetReaderIsInUse(LCCO_CARD *cd, int i);

time_t LCCM_Card_GetLastAdTime(const LCCO_CARD *cd);
void LCCM_Card_SetLastAdTime(LCCO_CARD *cd, time_t t);


GWEN_TYPE_UINT32 LCCM_Card_GetReaderLockId(const LCCO_CARD *cd);
void LCCM_Card_SetReaderLockId(LCCO_CARD *cd, GWEN_TYPE_UINT32 i);




#endif /* CHIPCARD_SERVER2_CM_CARD_L_H */

