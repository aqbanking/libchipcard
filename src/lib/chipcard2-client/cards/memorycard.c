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


#include "memorycard_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/text.h>
#include <chipcard2/chipcard2.h>


GWEN_INHERIT(LC_CARD, LC_MEMORYCARD)



int LC_MemoryCard_ExtendCard(LC_CARD *card){
  LC_MEMORYCARD *mc;

  GWEN_NEW_OBJECT(LC_MEMORYCARD, mc);

  mc->openFn=LC_Card_GetOpenFn(card);
  mc->closeFn=LC_Card_GetCloseFn(card);
  LC_Card_SetOpenFn(card, LC_MemoryCard_Open);
  LC_Card_SetCloseFn(card, LC_MemoryCard_Close);

  GWEN_INHERIT_SETDATA(LC_CARD, LC_MEMORYCARD, card, mc,
                       LC_MemoryCard_freeData);

  LC_MemoryCard__CalculateCapacity(card);

  return 0;
}



int LC_MemoryCard_UnextendCard(LC_CARD *card){
  LC_MEMORYCARD *mc;

  mc=GWEN_INHERIT_GETDATA(LC_CARD, LC_MEMORYCARD, card);
  assert(mc);
  LC_Card_SetOpenFn(card, mc->openFn);
  LC_Card_SetCloseFn(card, mc->closeFn);
  GWEN_INHERIT_UNLINK(LC_CARD, LC_MEMORYCARD, card);
  return 0;
}



void LC_MemoryCard_freeData(void *bp, void *p){
  LC_MEMORYCARD *mc;

  assert(bp);
  assert(p);
  mc=(LC_MEMORYCARD*)p;
  GWEN_FREE_OBJECT(mc);
}



LC_CLIENT_RESULT LC_MemoryCard_Open(LC_CARD *card){
  LC_CLIENT_RESULT res;
  LC_MEMORYCARD *mc;

  DBG_DEBUG(LC_LOGDOMAIN, "Opening card as memory card");

  assert(card);
  mc=GWEN_INHERIT_GETDATA(LC_CARD, LC_MEMORYCARD, card);
  assert(mc);

  res=mc->openFn(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  res=LC_MemoryCard_Reopen(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    mc->closeFn(card);
    return res;
  }

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_MemoryCard_Reopen(LC_CARD *card){
  LC_CLIENT_RESULT res;
  LC_MEMORYCARD *mc;
  GWEN_BUFFER *vbuf;
  int i;

  DBG_DEBUG(LC_LOGDOMAIN, "Opening memory card");

  assert(card);
  mc=GWEN_INHERIT_GETDATA(LC_CARD, LC_MEMORYCARD, card);
  assert(mc);

  DBG_DEBUG(LC_LOGDOMAIN, "Selecting memory card and app");
  res=LC_Card_SelectCardAndApp(card, "MemoryCard", "MemoryCard");
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  DBG_DEBUG(LC_LOGDOMAIN, "Selecting MF...");
  res=LC_Card_SelectMF(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  /* get driver variables */
  DBG_DEBUG(LC_LOGDOMAIN, "Getting value of WriteBoundary");
  vbuf=GWEN_Buffer_new(0, 256, 0, 1);
  res=LC_Card_GetDriverVar(card, "WriteBoundary", vbuf);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_Buffer_free(vbuf);
    return res;
  }
  if (GWEN_Buffer_GetUsedBytes(vbuf)==0)
    /* default value */
    GWEN_Buffer_AppendString(vbuf, LC_MEMORYCARD_DEFAULT_WRITEBOUNDARY_S);

  if (1!=sscanf(GWEN_Buffer_GetStart(vbuf), "%i", &i)) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad value for variable MaxWriteBinary (%s)",
              GWEN_Buffer_GetStart(vbuf));
    i=LC_MEMORYCARD_DEFAULT_WRITEBOUNDARY;
  }
  GWEN_Buffer_free(vbuf);
  mc->writeBoundary=i;

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_MemoryCard_Close(LC_CARD *card){
  LC_CLIENT_RESULT res;
  LC_MEMORYCARD *mc;

  assert(card);
  mc=GWEN_INHERIT_GETDATA(LC_CARD, LC_MEMORYCARD, card);
  assert(mc);

  res=mc->closeFn(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  return res;
}



GWEN_TYPE_UINT32 LC_MemoryCard__SendReadBinary(LC_CARD *card,
                                               int offset,
                                               unsigned int size){
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rid;

  DBG_DEBUG(LC_LOGDOMAIN, "Reading binary %04x:%04x", offset, size);

  dbReq=GWEN_DB_Group_new("ReadBinary");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "offset", offset);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "lr", size);

  rid=LC_Client_SendExecCommand(LC_Card_GetClient(card), card, dbReq);
  if (rid==0) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return 0;
  }

  return rid;
}



LC_CLIENT_RESULT LC_MemoryCard_ReadBinary(LC_CARD *card,
                                          int offset,
                                          int size,
                                          GWEN_BUFFER *buf){
  int t;
  LC_MEMORYCARD *mc;
  GWEN_TYPE_UINT32 request[LC_MEMORYCARD_MAXREQUESTS];
  int reqs;
  int i;
  LC_CLIENT *cl;

  assert(card);
  mc=GWEN_INHERIT_GETDATA(LC_CARD, LC_MEMORYCARD, card);
  assert(mc);

  cl=LC_Card_GetClient(card);
  assert(cl);

  while(size>0) {
    reqs=0;
    while(size>0 && reqs<LC_MEMORYCARD_MAXREQUESTS) {
      if (size>252)
        t=252;
      else
        t=size;
      request[reqs]=LC_MemoryCard__SendReadBinary(card, offset, t);
      if (request[reqs]==0) {
        DBG_INFO(LC_LOGDOMAIN, "here");
        return LC_Client_ResultGeneric;
      }
      reqs++;

      size-=t;
      offset+=t;
    } /* while */

    /* then wait for these requests to be handled */
    for (i=0; i<reqs; i++) {
      LC_CLIENT_RESULT res;
      GWEN_DB_NODE *dbResp;

      /* wait for response */
      DBG_DEBUG(LC_LOGDOMAIN, "Checking request %d", i);
      res=LC_Client_CheckResponse_Wait(cl,
                                       request[i],
                                       LC_Client_GetLongTimeout(cl));
      if (res!=LC_Client_ResultOk) {
        int j;

        DBG_INFO(LC_LOGDOMAIN, "here");
        for (j=i; j<reqs; j++)
          LC_Client_DeleteRequest(cl,
                                  request[j]);
        return res;
      }
      DBG_DEBUG(LC_LOGDOMAIN, "Request %d: ok.", i);
      dbResp=GWEN_DB_Group_new("resp");

      /* check response */
      res=LC_Client_CheckExecCommand(cl, request[i], dbResp);
      if (res!=LC_Client_ResultOk) {
        int j;

        DBG_INFO(LC_LOGDOMAIN, "here");
        for (j=i; j<reqs; j++)
          LC_Client_DeleteRequest(cl,
                                  request[j]);
        GWEN_DB_Group_free(dbResp);
        return res;
      }

      /* store data read */
      if (buf) {
        unsigned int bs;
        const void *p;

        p=GWEN_DB_GetBinValue(dbResp,
                              "command/response/data",
                              0,
                              0, 0,
                              &bs);
        if (p && bs) {
          GWEN_Buffer_AppendBytes(buf, p, bs);
        }
        else {
          DBG_WARN(LC_LOGDOMAIN, "No data in response");
        }
      }
      GWEN_DB_Group_free(dbResp);

      LC_Client_DeleteRequest(cl, request[i]);
      request[i]=0;
    } /* for */


  } /* while still data to read */


  return LC_Client_ResultOk;
}



GWEN_TYPE_UINT32 LC_MemoryCard__SendWriteBinary(LC_CARD *card,
                                                int offset,
                                                const char *ptr,
                                                unsigned int size){
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rid;

  DBG_DEBUG(LC_LOGDOMAIN, "Writing binary %04x:%04x", offset, size);

  dbReq=GWEN_DB_Group_new("WriteBinary");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "offset", offset);
  if (ptr) {
    if (size) {
      GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                          "data", ptr, size);
    }
  }

  rid=LC_Client_SendExecCommand(LC_Card_GetClient(card), card, dbReq);
  if (rid==0) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return 0;
  }

  return rid;
}



LC_CLIENT_RESULT LC_MemoryCard_WriteBinary(LC_CARD *card,
                                           int offset,
                                           const char *ptr,
                                           unsigned int size) {
  int rest;
  LC_MEMORYCARD *mc;
  GWEN_TYPE_UINT32 request[LC_MEMORYCARD_MAXREQUESTS];
  int reqs;
  int i;
  LC_CLIENT *cl;

  assert(card);
  mc=GWEN_INHERIT_GETDATA(LC_CARD, LC_MEMORYCARD, card);
  assert(mc);

  cl=LC_Card_GetClient(card);
  assert(cl);

  reqs=0;
  rest=size;

  while(rest) {
    /* first send a bunch of requests */
    reqs=0;
    while(rest && reqs<LC_MEMORYCARD_MAXREQUESTS) {
      int t;
      int j;

      /* calculate the position at the next boundary */
      j=(((offset)/mc->writeBoundary)+1)*mc->writeBoundary;
      /* read the distance from the current offset to that next boundary */
      t=j-(offset);

      if (t>rest)
        t=rest;

      DBG_DEBUG(LC_LOGDOMAIN, "Pushing WriteBinary (%04x, %d)",
                offset, t);
      request[reqs]=LC_MemoryCard__SendWriteBinary(card, offset, ptr, t);
      if (request[reqs]==0) {
        DBG_INFO(LC_LOGDOMAIN, "here");
        return LC_Client_ResultGeneric;
      }
      reqs++;
      rest-=t;
      offset+=t;
      ptr+=t;
    } /* while */

    /* then wait for these requests to be handled */
    for (i=0; i<reqs; i++) {
      LC_CLIENT_RESULT res;
      GWEN_DB_NODE *dbResp;

      DBG_DEBUG(LC_LOGDOMAIN, "Checking request %d", i);
      res=LC_Client_CheckResponse_Wait(cl,
                                       request[i],
                                       LC_Client_GetLongTimeout(cl));
      if (res!=LC_Client_ResultOk) {
        int j;

        DBG_INFO(LC_LOGDOMAIN, "here");
        for (j=i; j<reqs; j++)
          LC_Client_DeleteRequest(cl,
                                  request[j]);
        return res;
      }
      dbResp=GWEN_DB_Group_new("resp");
      res=LC_Client_CheckExecCommand(cl, request[i], dbResp);
      if (res!=LC_Client_ResultOk) {
        int j;

        DBG_INFO(LC_LOGDOMAIN, "here");
        for (j=i; j<reqs; j++)
          LC_Client_DeleteRequest(cl, request[j]);
        GWEN_DB_Group_free(dbResp);
        return res;
      }

      GWEN_DB_Group_free(dbResp);
      DBG_DEBUG(LC_LOGDOMAIN, "Request %d: ok.", i);
      LC_Client_DeleteRequest(cl, request[i]);
      request[i]=0;
    } /* for */
  } /* while still data to write */

  return LC_Client_ResultOk;
}



void LC_MemoryCard__CalculateCapacity(LC_CARD *card){
  LC_MEMORYCARD *mc;
  int i1, i2;
  int j1, j2;
  GWEN_BUFFER *atr;
  const unsigned char *p;

  assert(card);
  mc=GWEN_INHERIT_GETDATA(LC_CARD, LC_MEMORYCARD, card);
  assert(mc);

  mc->capacity=0;
  atr=LC_Card_GetAtr(card);
  if (!atr) {
    DBG_WARN(LC_LOGDOMAIN, "No ATR");
    return;
  }

  p=GWEN_Buffer_GetStart(atr);
  assert(p);
  if (GWEN_Buffer_GetUsedBytes(atr)<2) {
    DBG_WARN(LC_LOGDOMAIN, "ATR too small");
    return;
  }

  i1=(p[1]>>3) & 0x0f; /* count of elements */
  i2=p[1] & 0x07;      /* size of element */

  /* check element number */
  if (i1==0)
    j1=1;
  else
    j1=1<<i1;
  j1*=64;

  /* check element size */
  if (i2==0)
    j2=1;
  else
    j2=1<<i2;

  /* calculate memory size */
  if (j1 && j2)
    mc->capacity=j1*j2/8;
  DBG_DEBUG(LC_LOGDOMAIN, "Capacity is: %d", mc->capacity);
}



unsigned int LC_MemoryCard_GetCapacity(const LC_CARD *card){
  LC_MEMORYCARD *mc;

  assert(card);
  mc=GWEN_INHERIT_GETDATA(LC_CARD, LC_MEMORYCARD, card);
  assert(mc);

  return mc->capacity;
}







