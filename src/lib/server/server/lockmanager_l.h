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



#ifndef CHIPCARD_SERVER2_LOCKMANAGER_L_H
#define CHIPCARD_SERVER2_LOCKMANAGER_L_H

#include <gwenhywfar/types.h>


typedef struct LCS_LOCKMANAGER LCS_LOCKMANAGER;


/**
 * @param objectTypeName just for debugging (e.g. "Reader", "Card")
 */
LCS_LOCKMANAGER *LCS_LockManager_new(const char *objectTypeName);
void LCS_LockManager_free(LCS_LOCKMANAGER *lm);

const char *LCS_LockManager_GetObjectTypeName(const LCS_LOCKMANAGER *lm);

/**
 * Request a lock on the card. The locking time will be at most the number
 * of seconds given in the parameter <i>duration</i>.
 * @return 0 on error, lock request id otherwise
 * @param duration maximum number of seconds the lock will hold
 * @param maxLocks maximum number of locks allowed for this client
 */
GWEN_TYPE_UINT32 LCS_LockManager_RequestLock(LCS_LOCKMANAGER *lm,
                                             GWEN_TYPE_UINT32 clid,
                                             int duration,
                                             int maxLocks);

/**
 * Request a lock on the card. The locking time will be at most the number
 * of seconds given in the parameter <i>duration</i>.
 * @return -1 on error, o if ok
 * @param duration maximum number of seconds the lock will hold
 * @param maxLocks maximum number of locks allowed for this client
 */
int LCS_LockManager_RequestLockWithId(LCS_LOCKMANAGER *lm,
                                      GWEN_TYPE_UINT32 lockid,
                                      GWEN_TYPE_UINT32 clid,
                                      int duration,
                                      int maxLocks);

GWEN_TYPE_UINT32 LCS_LockManager_GetNextRequestId();


/**
 * @return -1 on error, 0 if request granted, 1 of not
 */
int LCS_LockManager_CheckRequest(LCS_LOCKMANAGER *lm,
                                 GWEN_TYPE_UINT32 reqid);

/**
 * Removes a lock applied by the combination of
 * @ref LCS_LockManager_RequestLock and @ref LCS_LockManager_CheckRequest.
 */
int LCS_LockManager_Unlock(LCS_LOCKMANAGER *lm, GWEN_TYPE_UINT32 reqid);


/**
 * Removes a lock request from the list of waiting requests.
 * The request to be removed MUST NOT be the currently active one (use
 * @ref LCS_LockManager_Unlock in this case).
 * @return 0 if ok, !=0 on error
 */
int LCS_LockManager_RemoveRequest(LCS_LOCKMANAGER *lm,
                                  GWEN_TYPE_UINT32 reqid);


/**
 * Removes all lock requests for the given client. If the client already
 * acquired a lock on this card then it will be removed (thus unlocking the
 * card for use by other clients).
 */
void LCS_LockManager_RemoveAllClientRequests(LCS_LOCKMANAGER *lm,
                                             GWEN_TYPE_UINT32 clid);

int LCS_LockManager_HasLockRequests(const LCS_LOCKMANAGER *lm);


int LCS_LockManager_CheckAccess(LCS_LOCKMANAGER *lm,
                                GWEN_TYPE_UINT32 reqid);


#endif
