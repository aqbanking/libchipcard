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


#include "cardmanager_p.h"

#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


static GWEN_TYPE_UINT32 lccm_cardmanager__last_card_id=0;


LCCM_CARDMANAGER *LCCM_CardManager_new(LCS_SERVER *server) {
  LCCM_CARDMANAGER *cm;

  GWEN_NEW_OBJECT(LCCM_CARDMANAGER, cm);
  cm->server=server;
  cm->deviceManager=LCS_Server_GetDeviceManager(cm->server);
  assert(cm->deviceManager);
  cm->cards=LCCO_Card_List_new();

  return cm;
}



void LCCM_CardManager_free(LCCM_CARDMANAGER *cm) {
  if (cm) {
    LCCO_Card_List_free(cm->cards);
    GWEN_FREE_OBJECT(cm);
  }
}



int LCCM_CardManager_Init(LCCM_CARDMANAGER *cm, GWEN_DB_NODE *dbConfig) {
  GWEN_DB_NODE *dbT;

  DBG_INFO(0, "Initializing card manager");
  assert(cm);

  cm->unusedTimeout=LCCM_CARDMANAGER_DEF_UNUSED_TIMEOUT;
  cm->minimumKeepTime=LCCM_CARDMANAGER_DEF_MINIMUM_KEEP_TIME;
  dbT=GWEN_DB_GetGroup(dbConfig, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                       "CardManager");
  if (dbT) {
    /* read some timeout values */
#define LCCM_CM_INIT_TIME(s) \
  cm->s=GWEN_DB_GetIntValue(dbT, __STRING(s), 0, cm->s);
    LCCM_CM_INIT_TIME(unusedTimeout)
    LCCM_CM_INIT_TIME(minimumKeepTime)
#undef LCDM_DM_INIT_TIME
  }
  return 0;
}



int LCCM_CardManager_Fini(LCCM_CARDMANAGER *cm, GWEN_DB_NODE *dbConfig) {
  assert(cm);

  LCCO_Card_List_Clear(cm->cards);

  return 0;
}



void LCCM_CardManager__RemoveCardsInSlots(LCCM_CARDMANAGER *cm,
                                          GWEN_TYPE_UINT32 rid,
                                          int slotNum) {
  LCCO_CARD *card;

  card=LCCO_Card_List_First(cm->cards);
  while(card) {
    if (LCCO_Card_GetReaderId(card)==rid &&
        LCCO_Card_GetSlotNum(card)==slotNum) {
      if (LCCO_Card_GetStatus(card)==LC_CardStatusInserted){
        LCCO_Card_SetStatus(card, LC_CardStatusRemoved);
      }
    }
    card=LCCO_Card_List_Next(card);
  }
}



void LCCM_CardManager__RemoveCardsInReader(LCCM_CARDMANAGER *cm,
                                           GWEN_TYPE_UINT32 rid) {
  LCCO_CARD *card;

  card=LCCO_Card_List_First(cm->cards);
  while(card) {
    if (LCCO_Card_GetReaderId(card)==rid) {
      if (LCCO_Card_GetStatus(card)==LC_CardStatusInserted){
        LCCO_Card_SetStatus(card, LC_CardStatusRemoved);
      }
    }
    card=LCCO_Card_List_Next(card);
  }
}



void LCCM_CardManager_NewCard(LCCM_CARDMANAGER *cm, LCCO_CARD *card) {
  assert(cm);
  assert(card);

  LCCO_Card_Attach(card);
  LCCM_Card_extend(card);
  LCCO_Card_SetCardId(card, ++lccm_cardmanager__last_card_id);

  LCDM_DeviceManager_BeginUseCard(cm->deviceManager, card);
  LCCM_Card_SetReaderIsInUse(card, 1);

  LCCM_CardManager__RemoveCardsInSlots(cm,
                                       LCCO_Card_GetReaderId(card),
                                       LCCO_Card_GetSlotNum(card));
  DBG_NOTICE(0, "New card added");
  LCCO_Card_List_Add(card, cm->cards);
}



void LCCM_CardManager_CardRemoved(LCCM_CARDMANAGER *cm,
                                  GWEN_TYPE_UINT32 rid,
                                  int slotNum,
                                  GWEN_TYPE_UINT32 cardNum) {
  LCCO_CARD *card;

  card=LCCO_Card_List_First(cm->cards);
  while(card) {
    if (LCCO_Card_GetReaderId(card)==rid &&
        LCCO_Card_GetSlotNum(card)==slotNum &&
        LCCO_Card_GetReadersCardId(card)==cardNum) {
      if (LCCO_Card_GetStatus(card)==LC_CardStatusInserted){
        DBG_NOTICE(0, "===================== Card %08x/%d/%08x removed",
                 rid, slotNum, cardNum);
        LCCO_Card_SetStatus(card, LC_CardStatusRemoved);
      }
      break;
    }
    card=LCCO_Card_List_Next(card);
  }
}



LCCO_CARD *LCCM_CardManager_FindCard(LCCM_CARDMANAGER *cm,
                                     GWEN_TYPE_UINT32 cid) {
  LCCO_CARD *card;

  card=LCCO_Card_List_First(cm->cards);
  while(card) {
    if (LCCO_Card_GetCardId(card)==cid)
      break;
    card=LCCO_Card_List_Next(card);
  }

  return card;
}



int LCCM_CardManager_CheckAccess(LCCM_CARDMANAGER *cm,
                                 LCCO_CARD *card,
                                 GWEN_TYPE_UINT32 clid) {
  assert(cm);
  assert(card);
  return LCCM_Card_CheckAccess(card, clid);
}



LCCO_CARD *LCCM_CardManager_GetFirstCard(LCCM_CARDMANAGER *cm) {
  LCCO_CARD *card;

  assert(cm);

  card=LCCO_Card_List_First(cm->cards);
  if (card)
    return card;

  return 0;
}



LCCO_CARD *LCCM_CardManager_GetNextCard(LCCM_CARDMANAGER *cm,
                                        LCCO_CARD *card) {
  assert(cm);
  assert(card);

  card=LCCO_Card_List_Next(card);
  if (card)
    return card;

  return 0;
}



int LCCM_CardManager_RequestLockCard(LCCM_CARDMANAGER *cm,
                                     LCCO_CARD *card,
                                     GWEN_TYPE_UINT32 clid,
                                     int duration,
                                     int maxLocks) {
  assert(cm);
  assert(card);

  return LCCM_Card_RequestLock(card, clid, duration, maxLocks);
}



int LCCM_CardManager_CheckLockCardRequest(LCCM_CARDMANAGER *cm,
                                          LCCO_CARD *card,
                                          GWEN_TYPE_UINT32 clid) {
  assert(cm);
  assert(card);

  return LCCM_Card_CheckRequest(card, clid);
}



int LCCM_CardManager_UnlockCard(LCCM_CARDMANAGER *cm,
                                LCCO_CARD *card,
                                GWEN_TYPE_UINT32 clid) {
  assert(cm);
  assert(card);

  return LCCM_Card_Unlock(card, clid);
}



int LCCM_CardManager_SetCardAdTime(LCCM_CARDMANAGER *cm,
                                   LCCO_CARD *card,
                                   time_t t) {
  assert(cm);
  assert(card);

  LCCM_Card_SetLastAdTime(card, t);
  return 0;
}



void LCCM_CardManager_ReaderDown(LCCM_CARDMANAGER *cm, GWEN_TYPE_UINT32 rid){
  assert(cm);
  LCCM_CardManager__RemoveCardsInReader(cm, rid);
}



void LCCM_CardManager_ClientDown(LCCM_CARDMANAGER *cm, GWEN_TYPE_UINT32 clid){
  LCCO_CARD *card;

  assert(cm);
  card=LCCO_Card_List_First(cm->cards);
  while(card) {
    LCCM_Card_RemoveAllClientRequests(card, clid);
    card=LCCO_Card_List_Next(card);
  }
}



int LCCM_CardManager_Work(LCCM_CARDMANAGER *cm){
  LCCO_CARD *card;
  int done=0;

  assert(cm);

  DBG_ERROR(0, "CardManager: Working");

  card=LCCO_Card_List_First(cm->cards);
  while(card) {
    LCCO_CARD *next;
    LC_CARD_STATUS currSt;
    LC_CARD_STATUS lastSt;

    next=LCCO_Card_List_Next(card);

    currSt=LCCO_Card_GetStatus(card);
    lastSt=LCCM_Card_GetLastStatus(card);

    if (currSt!=lastSt) {
      /* card status changed, this also sets the unusedSince time */
      DBG_ERROR(0, "Card status changed");
      LCCM_Card_SetLastStatus(card, currSt);
    }

    /* check timeouts */
    if (LCCM_Card_GetReaderIsInUse(card)) {
      time_t t;

      t=LCCM_Card_GetUnusedSince(card);
      if (t) {
        time_t t1;

        t1=time(0);
        if (difftime(t1, t)>cm->unusedTimeout) {
          DBG_NOTICE(0, "Card too long unused");
          /* check second timeout */
          t=LCCM_Card_GetLastAdTime(card);
          t1=time(0);
          if (difftime(t1, t)>cm->minimumKeepTime) {
            DBG_NOTICE(0, "Keep time exceeded");

            DBG_NOTICE(0, "Allowing reader to shut down");
            /* this does not immediately shut down the reader, but as long
             * as this function has not been called the reader will not be
             * allowed to shut down.
             * Effectively this adds a grace period for inserted cards, so
             * if there is a card in a reader (even if it is not used or
             * requested by any client) the reader will not shut down for a
             * certain amount of seconds (adjustable in config file) thus
             * allowing a client to later use this card. */
            LCDM_DeviceManager_EndUseCard(cm->deviceManager, card);
            LCCM_Card_SetReaderIsInUse(card, 0);
            done++;
          }
        }
      }
    }

    if (currSt!=LC_CardStatusInserted) {
      if (currSt!=lastSt) {
        DBG_NOTICE(0, "Card %08x/%d/%08x removed",
                   LCCO_Card_GetReaderId(card),
                   LCCO_Card_GetSlotNum(card),
                   LCCO_Card_GetReadersCardId(card));
      }

      if (LCCM_Card_GetReaderIsInUse(card)) {
        /* card was still in use, end usage. Otherwise we would always
         * trigger the start of this reader even though this card will
         * never come back to "inserted" status (because after a reader
         * start the card gets a new id) */
        LCDM_DeviceManager_EndUseCard(cm->deviceManager, card);
        LCCM_Card_SetReaderIsInUse(card, 0);
        done++;
      }
      if (LCCM_Card_HasLockRequests(card)==0) {
        /* card is not inserted and has no lock requests, remove it */
        DBG_NOTICE(0, "Free card no longer inserted, removing from list");
        LCCO_Card_List_Del(card);
        LCCO_Card_free(card);
        card=0;
        done++;
      }
    }
    else {
      /* card still inserted */
      if (LCCM_Card_GetReaderIsInUse(card)==0) {
        int useAgain=0;
        time_t t1;
        time_t t;

        /* check whether we have to allocate the reader again.
         * We do so if either the keep time counter has been restarted
         * (by a client calling LCCM_CardManager_SetCardAdTime) or the
         * card is now in use. */
        t1=time(0);
        t=LCCM_Card_GetLastAdTime(card);
        if (t) {
          if (difftime(t1, t)<cm->minimumKeepTime) {
            DBG_NOTICE(0, "Keep time counter restarted");
            useAgain=1;
          }
        }

        if (LCCM_Card_HasLockRequests(card)!=0 &&
            LCCM_Card_GetReaderIsInUse(card)==0) {
          DBG_NOTICE(0, "Card now in use");
          useAgain=1;
        }

        if (useAgain) {
          /* the card is now in use, but we already allowed the reader
           * to shut down. However, the card is still inserted so we just
           * no longer allow the reader to shut down. */
          DBG_NOTICE(0, "No longer allowing reader to shut down");
          LCDM_DeviceManager_BeginUseCard(cm->deviceManager, card);
          LCCM_Card_SetReaderIsInUse(card, 1);
          done++;
        }
      }
    }
    card=next;
  } /* while */

  if (done)
    return 1;
  return 0;
}








