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


#include "cardserver_p.h"
#include "serverconn_l.h"
#include "card_l.h"
#include <gwenhywfar/version.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/ipc.h>
#include <gwenhywfar/nettransportssl.h>
#include <gwenhywfar/nettransportsock.h>
#include <gwenhywfar/net.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/directory.h>

#include <chipcard2/chipcard2.h>
#include <chipcard2-server/server/usbmonitor.h>
#include <chipcard2-server/server/usbttymonitor.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>


#ifdef OS_WIN32
# define DIRSEP "\\"
# define DIRSEPC '\\'
#else
# define DIRSEP "/"
# define DIRSEPC '/'
#endif




void LC_CardServer_CardDown(LC_CARDSERVER *cs, LC_CARD *card,
                            LC_CARD_STATUS newCardStatus,
                            const char *reason) {
  LC_REQUEST *rq;
  LC_CARD_STATUS cst;

  cst=LC_Card_GetStatus(card);

  DBG_NOTICE(0, "Card %08x is down: %s", LC_Card_GetCardId(card),
             reason);

  /* discard all requests concerning this card */
  rq=LC_Request_List_First(cs->requests);
  while(rq) {
    LC_REQUEST *rqnext;

    rqnext=LC_Request_List_Next(rq);
    if (LC_Request_GetCard(rq)==card) {
      LC_CardServer_SendErrorResponse(cs, LC_Request_GetInRequestId(rq),
                                      LC_ERROR_CARD_REMOVED,
                                      reason);
      DBG_INFO(0, "Aborting cards' request %08x",
               LC_Request_GetRequestId(rq));
      if (1) {
        GWEN_DB_NODE *dbReq;

        dbReq=LC_Request_GetInRequestData(rq);
        assert(dbReq);
        GWEN_DB_Dump(dbReq, stderr, 4);
      }
      LC_CardServer_RemoveRequest(cs, rq);
    }
    rq=rqnext;
  }
  LC_Card_ClearWaitingClients(card);

  if (cst!=newCardStatus) {
    DBG_INFO(0, "Card %08x down",
             LC_Card_GetCardId(card));
    LC_Card_SetStatus(card, newCardStatus);
  }
}



int LC_CardServer_CheckCards(LC_CARDSERVER *cs) {
  LC_CARD *cd;
  int needIO;

  assert(cs);
  needIO=0;

  DBG_VERBOUS(0, "Checking active cards");
  cd=LC_Card_List_First(cs->activeCards);
  while(cd) {
    LC_CARD *next;
    LC_READER *r;
    LC_READER_STATUS rst;

    next=LC_Card_List_Next(cd);

    r=LC_Card_GetReader(cd);
    assert(r);
    rst=LC_Reader_GetStatus(r);

    if (rst==LC_ReaderStatusUp || rst==LC_ReaderStatusWaitForReaderUp) {
      /* TODO: Check for busy timeout to prevent one user blocking a card */
    }
    cd=next;
  } /* while */

  DBG_VERBOUS(0, "Checking free cards");
  cd=LC_Card_List_First(cs->freeCards);
  while(cd) {
    LC_CARD *next;
    LC_READER *r;
    LC_READER_STATUS rst;

    next=LC_Card_List_Next(cd);

    r=LC_Card_GetReader(cd);
    assert(r);
    rst=LC_Reader_GetStatus(r);

    if (LC_Card_GetStatus(cd)!=LC_CardStatusInserted) {
      DBG_NOTICE(0,
                 "Unused card \"%08x\" in reader \"%s\" is invalid, "
                 "removing it",
                 LC_Card_GetCardId(cd),
                 LC_Reader_GetReaderName(r));
      /* remove card from list of free cards, free it */
      LC_Card_List_Del(cd);
      LC_Card_free(cd);
    }
    else {
      if (rst==LC_ReaderStatusUp ||
          rst==LC_ReaderStatusWaitForReaderUp) {
        LC_CLIENT *cl;

        for (;;) {
          GWEN_TYPE_UINT32 cid;

          /* check whether someone is already waiting for this card */
          cid=LC_Card_GetFirstWaitingClient(cd);
          if (cid) {
            /* yes, in fact there is */
            cl=LC_CardServer_FindClient(cs, cid);
            if (cl) {
              LC_REQUEST *rq;

              rq=LC_CardServer_FindClientCardRequest(cs, cl, cd,
                                                     "TakeCard");
              if (rq) {
                GWEN_DB_NODE *dbRsp;

                DBG_DEBUG(0, "Found a TakeCard request (id=%d)",
                          LC_Request_GetInRequestId(rq));
                needIO++;
                dbRsp=GWEN_DB_Group_new("TakeCardResponse");
                GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_DEFAULT,
                                     "code", "OK");
                GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_DEFAULT,
                                     "text", "the card is yours");
                if (GWEN_IPCManager_SendResponse(cs->ipcManager,
                                                 LC_Request_GetInRequestId(rq),
                                                 dbRsp)) {
                  DBG_ERROR(0, "Could not send TakeCard response");
                  LC_Reader_SetStatus(r, LC_ReaderStatusAborted);
                  LC_CardServer_SendReaderNotification(cs, 0,
                                                       LC_NOTIFY_CODE_READER_ERROR,
                                                       r,
                                                       "Could not send "
                                                       "TakeCard response");
                  GWEN_IPCManager_RemoveRequest(cs->ipcManager,
                                                LC_Request_GetInRequestId(rq),
                                                0);
                }
                else {
                  /* card is now in clients hands */
                  DBG_NOTICE(0,
                             "Card \"%08x\" taken by client \"%08x\"",
                             LC_Card_GetCardId(cd),
                             LC_Client_GetClientId(cl));
                  LC_Card_SetClient(cd, cl);

                  /* move card from freeCards to activeCards */
                  LC_Card_List_Del(cd);
                  LC_Card_List_Add(cd, cs->activeCards);

                  /* remove client from waiting list */
                  LC_Card_DelWaitingClient(cd, cid);
                  /* card is sold, break the loop */
                  DBG_INFO(0, "Card is sold to client \"%08x\"", cid);
                  /* remove request from list and free it */
                  GWEN_IPCManager_RemoveRequest(cs->ipcManager,
                                                LC_Request_GetInRequestId(rq),
                                                0);
                  LC_Request_List_Del(rq);
                  LC_Request_free(rq);
                  break;
                }
              }
              else {
                /* remove client from waiting list */
                DBG_WARN(0,
                         "Did not find a TakeCard request for client \"%08x\"",
                         LC_Client_GetClientId(cl));
                LC_Card_DelWaitingClient(cd, LC_Client_GetClientId(cl));
                needIO++;
              }
            } /* if client found */
            else {
              /* client not found, remove this id from waiting list */
              DBG_WARN(0, "Did not find client \"%08x\"", cid);
              LC_Card_DelWaitingClient(cd, cid);

              needIO++;
            }
          } /* if there was a waiting client */
          else
            break;
        } /* for */

        /* reader is up, check whether this card is wanted by anyone */
        cl=LC_Client_List_First(cs->clients);
        while(cl) {
          if (LC_Client_HasReader(cl, LC_Reader_GetReaderId(r)) &&
              !LC_Client_HasCard(cl, LC_Card_GetCardId(cd))) {
            LC_DRIVER *d;
            GWEN_DB_NODE *gr;
            const char *readerType;
            const char *driverName;
            GWEN_BUFFER *atr;
            char numbuf[16];
            GWEN_TYPE_UINT32 flags;
            LC_CARD_TYPE ct;
            GWEN_TYPE_UINT32 rid;
            GWEN_STRINGLISTENTRY *se;

            /* card is newly available to this client */
            needIO++;
            DBG_NOTICE(0, "Advertising card %08x to client %08x",
                       LC_Card_GetCardId(cd),
                       LC_Client_GetClientId(cl));
            d=LC_Reader_GetDriver(r);
            assert(d);
            gr=GWEN_DB_Group_new("CardAvailable");

            readerType=LC_Reader_GetReaderType(r);
            assert(readerType);
            driverName=LC_Driver_GetDriverName(d);
            atr=LC_Card_GetAtr(cd);
            snprintf(numbuf, sizeof(numbuf)-1, "%08x",
                     LC_Card_GetCardId(cd));
            numbuf[sizeof(numbuf)-1]=0;

            GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_OVERWRITE_VARS,
                                 "cardid", numbuf);
            GWEN_DB_DeleteVar(gr, "readerflags");
            flags=LC_Reader_GetFlags(r);
            if (flags & LC_READER_FLAGS_KEYPAD)
              GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                                   "readerflags", "KEYPAD");
            if (flags & LC_READER_FLAGS_DISPLAY)
              GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                                   "readerflags", "DISPLAY");
            if (flags & LC_READER_FLAGS_NOINFO)
              GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                                   "readerflags", "NOINFO");
            if (flags & LC_READER_FLAGS_REMOTE)
              GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                                   "readerflags", "REMOTE");
            if (flags & LC_READER_FLAGS_AUTO)
              GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                                   "readerflags", "AUTO");

            /* add card types */
            se=GWEN_StringList_FirstEntry(LC_Card_GetTypes(cd));
            while (se) {
              const char *s;

              s=GWEN_StringListEntry_Data(se);
              assert(s);
              GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                                   "cardtypes", s);
              se=GWEN_StringListEntry_Next(se);
            }

            /* add cards' base type */
            ct=LC_Card_GetType(cd);
            if (ct==LC_CardTypeProcessor) {
              GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                                   "cardtypes", "ProcessorCard");
              GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                                   "cardtype", "PROCESSOR");
            }
            else if (ct==LC_CardTypeMemory) {
              GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                                   "cardtypes", "MemoryCard");
              GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                                   "cardtype", "MEMORY");
            }
            else
              GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                                   "cardtype", "UNKNOWN");

            if (atr) {
              if (GWEN_Buffer_GetUsedBytes(atr)) {
                GWEN_DB_SetBinValue(gr, GWEN_DB_FLAGS_OVERWRITE_VARS,
                                    "atr",
                                    GWEN_Buffer_GetStart(atr),
                                    GWEN_Buffer_GetUsedBytes(atr));
              } /* if ATR not empty */
            } /* if ATR */

            rid=GWEN_IPCManager_SendRequest(cs->ipcManager,
                                            LC_Client_GetClientId(cl),
                                            gr);
            if (rid==0) {
              DBG_ERROR(0, "Could not send \"CardAvailable\" to client");
            }
            else {
              /* client soon wil know about the card, add it to his list
               * to avoid sending a notification again */
              LC_Client_AddCard(cl, LC_Card_GetCardId(cd));
              /* remove request, we don't expect an answer */
              GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);
            }
          } /* if card is new to the client */
          cl=LC_Client_List_Next(cl);
        } /* while */
      } /* if reader is up */
    } /* if card is inserted */
    cd=next;
  } /* while */
  DBG_VERBOUS(0, "Checking free cards done (%d)", needIO);

  return needIO?0:1;
}



int LC_CardServer_RemoveCardsAt(LC_CARDSERVER *cs,
                                LC_READER *r,
                                unsigned int slotNum,
                                const char *reason) {
  LC_CARD *card;

  /* find card in active ones */
  card=LC_Card_List_First(cs->activeCards);
  while(card) {
    if (LC_Card_GetReader(card)==r &&
        LC_Card_GetSlot(card)==slotNum) {
      /* we got a card which was at the place of the new card */
      if (LC_Card_GetStatus(card)!=LC_CardStatusRemoved) {
        DBG_WARN(0, "Active card \"%08x\" removed",
                 LC_Card_GetCardId(card));
        LC_CardServer_CardDown(cs, card,
                               LC_CardStatusRemoved,
                               reason);
      }
    }
    card=LC_Card_List_Next(card);
  } /* while */

  /* find card in free ones */
  card=LC_Card_List_First(cs->freeCards);
  while(card) {
    LC_CARD *next;

    next=LC_Card_List_Next(card);
    if (LC_Card_GetReader(card)==r &&
        LC_Card_GetSlot(card)==slotNum) {
      /* we got a card which was at the place of the new card */
      if (LC_Card_GetStatus(card)!=LC_CardStatusRemoved) {
        DBG_WARN(0, "Unused card \"%08x\" removed",
                 LC_Card_GetCardId(card));
        LC_CardServer_CardDown(cs, card,
                               LC_CardStatusRemoved,
                               reason);
      }
    }
    card=next;
  } /* while */

  return 0;
}



int LC_CardServer_HandleCardInserted(LC_CARDSERVER *cs,
				     GWEN_TYPE_UINT32 rid,
				     GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  GWEN_TYPE_UINT32 nodeId;
  LC_READER *r;
  int slotNum;
  GWEN_TYPE_UINT32 cardNum;
  LC_CARD *card;
  GWEN_BUFFER *atr;
  const void *p;
  unsigned int bs;
  const char *cardType;
  LC_CARD_TYPE ct;

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }

  DBG_INFO(0, "Driver %08x: Card inserted", nodeId);

  /* driver ready */
  if (sscanf(GWEN_DB_GetCharValue(dbReq, "body/readerId", 0, "0"),
             "%x",
	     &readerId)!=1) {
    DBG_ERROR(0, "Bad reader id in command \"%s\"",
              GWEN_DB_GroupName(dbReq));
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }

  slotNum=GWEN_DB_GetIntValue(dbReq, "body/slotNum", 0, 0),
  cardNum=GWEN_DB_GetIntValue(dbReq, "body/cardNum", 0, 0),
  cardType=GWEN_DB_GetCharValue(dbReq, "body/cardType", 0, "unknown");
  if (strcasecmp(cardType, "processor")==0)
    ct=LC_CardTypeProcessor;
  else if (strcasecmp(cardType, "memory")==0)
    ct=LC_CardTypeMemory;
  else {
    DBG_WARN(0, "Unknown card type \"%s\"", cardType);
    ct=LC_CardTypeUnknown;
  }
  atr=0;
  p=GWEN_DB_GetBinValue(dbReq, "body/atr", 0, 0, 0, &bs);
  if (p && bs) {
    atr=GWEN_Buffer_new(0, bs, 0, 1);
    GWEN_Buffer_AppendBytes(atr, p, bs);
  }

  /* find reader */
  r=LC_Reader_List_First(cs->readers);
  while(r) {
    if (LC_Reader_GetReaderId(r)==readerId)
      break;
    r=LC_Reader_List_Next(r);
  } /* while */
  if (!r) {
    DBG_ERROR(0, "Reader \"%08x\" not found", readerId);
    GWEN_Buffer_free(atr);
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }

  /* TODO: Check for reader status */

  LC_CardServer_RemoveCardsAt(cs, r, slotNum, "Replaced by new card");

  card=LC_Card_new(r, slotNum, cardNum, ct, atr);
  LC_Card_SetStatus(card, LC_CardStatusInserted);
  LC_CardMgr_AddCardTypesByAtr(cs->cardManager, card);
  LC_Card_List_Add(card, cs->freeCards);
  DBG_NOTICE(0, "Free card added with id \"%08x\" in reader \"%s\"(%08x)",
             LC_Card_GetCardId(card),
             LC_Reader_GetReaderName(r),
             readerId);
  GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
  return 0;
}



int LC_CardServer_HandleCardRemoved(LC_CARDSERVER *cs,
				    GWEN_TYPE_UINT32 rid,
				    GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  GWEN_TYPE_UINT32 nodeId;
  LC_READER *r;
  int slotNum;
  GWEN_TYPE_UINT32 cardNum;

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }

  DBG_NOTICE(0, "Driver %08x: Card removed", nodeId);

  /* driver ready */
  if (sscanf(GWEN_DB_GetCharValue(dbReq, "body/readerId", 0, "0"),
             "%x",
             &readerId)!=1) {
    DBG_ERROR(0, "Bad reader id in command \"%s\"",
	      GWEN_DB_GroupName(dbReq));
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }

  slotNum=GWEN_DB_GetIntValue(dbReq, "body/slotNum", 0, 0),
  cardNum=GWEN_DB_GetIntValue(dbReq, "body/cardNum", 0, 0),

  /* find reader */
  r=LC_Reader_List_First(cs->readers);
  while(r) {
    if (LC_Reader_GetReaderId(r)==readerId)
      break;
    r=LC_Reader_List_Next(r);
  } /* while */
  if (!r) {
    DBG_ERROR(0, "Reader \"%08x\" not found", readerId);
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }

  /* TODO: Check for reader status */

  LC_CardServer_RemoveCardsAt(cs, r, slotNum, "Card removed");

  GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
  return 0;
}



GWEN_TYPE_UINT32 LC_CardServer_SendResetCard(LC_CARDSERVER *cs,
                                             const LC_CARD *card){
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  LC_READER *r;
  LC_DRIVER *d;
  unsigned int slot;
  unsigned int cardNum;

  DBG_NOTICE(0, "Resetting card \"%08x\"", LC_Card_GetCardId(card));
  r=LC_Card_GetReader(card);
  assert(r);
  d=LC_Reader_GetDriver(r);
  assert(d);
  slot=LC_Card_GetSlot(card);
  cardNum=LC_Card_GetReadersCardId(card);

  dbReq=GWEN_DB_Group_new("ResetCard");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LC_Reader_GetReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "slotnum", slot);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cardnum", cardNum);

  return GWEN_IPCManager_SendRequest(cs->ipcManager,
                                     LC_Driver_GetIpcId(d),
                                     dbReq);
}



LC_CARD *LC_CardServer_FindActiveCard(const LC_CARDSERVER *cs,
                                      GWEN_TYPE_UINT32 cid){
  LC_CARD *card;

  card=LC_Card_List_First(cs->activeCards);
  while(card) {
    if (LC_Card_GetCardId(card)==cid)
      break;
    card=LC_Card_List_Next(card);
  } /* while */

  return 0;
}







