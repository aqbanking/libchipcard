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


#include "processorcard_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/text.h>
#include <chipcard2-client/chipcard2.h>


GWEN_INHERIT(LC_CARD, LC_PROCESSORCARD)



int LC_ProcessorCard_ExtendCard(LC_CARD *card){
  LC_PROCESSORCARD *mc;

  GWEN_NEW_OBJECT(LC_PROCESSORCARD, mc);

  mc->openFn=LC_Card_GetOpenFn(card);
  mc->closeFn=LC_Card_GetCloseFn(card);
  LC_Card_SetOpenFn(card, LC_ProcessorCard_Open);
  LC_Card_SetCloseFn(card, LC_ProcessorCard_Close);

  GWEN_INHERIT_SETDATA(LC_CARD, LC_PROCESSORCARD, card, mc,
                       LC_ProcessorCard_freeData);
  return 0;
}



int LC_ProcessorCard_UnextendCard(LC_CARD *card){
  LC_PROCESSORCARD *mc;

  mc=GWEN_INHERIT_GETDATA(LC_CARD, LC_PROCESSORCARD, card);
  assert(mc);
  LC_Card_SetOpenFn(card, mc->openFn);
  LC_Card_SetCloseFn(card, mc->closeFn);
  GWEN_INHERIT_UNLINK(LC_CARD, LC_PROCESSORCARD, card);
  return 0;
}



void LC_ProcessorCard_freeData(void *bp, void *p){
  LC_PROCESSORCARD *mc;

  assert(bp);
  assert(p);
  mc=(LC_PROCESSORCARD*)p;
  GWEN_FREE_OBJECT(mc);
}



LC_CLIENT_RESULT LC_ProcessorCard_Open(LC_CARD *card){
  LC_CLIENT_RESULT res;
  LC_PROCESSORCARD *mc;

  DBG_DEBUG(LC_LOGDOMAIN, "Opening card as memory card");

  assert(card);
  mc=GWEN_INHERIT_GETDATA(LC_CARD, LC_PROCESSORCARD, card);
  assert(mc);

  res=mc->openFn(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  res=LC_ProcessorCard_Reopen(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_ProcessorCard_Reopen(LC_CARD *card){
  LC_CLIENT_RESULT res;
  LC_PROCESSORCARD *mc;

  DBG_DEBUG(LC_LOGDOMAIN, "Opening processor card");

  assert(card);
  mc=GWEN_INHERIT_GETDATA(LC_CARD, LC_PROCESSORCARD, card);
  assert(mc);

  DBG_DEBUG(LC_LOGDOMAIN, "Selecting processor card and app");
  res=LC_Card_SelectCardAndApp(card, "ProcessorCard", "ProcessorCard");
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_ProcessorCard_Close(LC_CARD *card){
  LC_CLIENT_RESULT res;
  LC_PROCESSORCARD *mc;

  assert(card);
  mc=GWEN_INHERIT_GETDATA(LC_CARD, LC_PROCESSORCARD, card);
  assert(mc);

  res=mc->closeFn(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  return res;
}



LC_CLIENT_RESULT LC_ProcessorCard_SelectDF(LC_CARD *card,
                                           const char *fname){
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;

  dbReq=GWEN_DB_Group_new("SelectDF");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                       "fname", fname);
  res=LC_Card_ExecCommand(card, dbReq, dbResp,
                          LC_Client_GetShortTimeout(LC_Card_GetClient(card)));
  GWEN_DB_Group_free(dbReq);
  GWEN_DB_Group_free(dbResp);
  return res;
}



LC_CLIENT_RESULT LC_ProcessorCard_SelectEF(LC_CARD *card,
                                           const char *fname){
  GWEN_XMLNODE *n;
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;

  assert(card);
  n=LC_Card_GetEfInfo(card);
  if (n) {
    const char *s;

    s=GWEN_XMLNode_GetProperty(n, "name", 0);
    if (s) {
      if (strcasecmp(s, fname)==0) {
        DBG_NOTICE(LC_LOGDOMAIN, "File already selected.");
        return LC_Client_ResultOk;
      }
    }
  }

  dbReq=GWEN_DB_Group_new("SelectEF");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                       "fname", fname);
  res=LC_Card_ExecCommand(card, dbReq, dbResp,
                          LC_Client_GetShortTimeout(LC_Card_GetClient(card)));
  GWEN_DB_Group_free(dbReq);
  GWEN_DB_Group_free(dbResp);
  return res;
}



LC_CLIENT_RESULT LC_ProcessorCard_ReadRecord(LC_CARD *card,
                                             int recNum,
                                             GWEN_BUFFER *buf){
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;
  unsigned int bs;
  const void *p;

  dbReq=GWEN_DB_Group_new("ReadRecord");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "recNum", recNum);
  res=LC_Card_ExecCommand(card, dbReq, dbResp,
                          LC_Client_GetShortTimeout(LC_Card_GetClient(card)));
  if (res!=LC_Client_ResultOk) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    return res;
  }

  /* successful */
  if (buf) {
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
  GWEN_DB_Group_free(dbReq);
  return res;
}



LC_CLIENT_RESULT LC_ProcessorCard_WriteRecord(LC_CARD *card,
                                              int recNum,
                                              GWEN_BUFFER *buf){
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;

  dbReq=GWEN_DB_Group_new("WriteRecord");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "recNum", recNum);
  if (buf) {
    if (GWEN_Buffer_GetUsedBytes(buf)) {
      GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                          "data",
                          GWEN_Buffer_GetStart(buf),
                          GWEN_Buffer_GetUsedBytes(buf));
    }
  }
  res=LC_Card_ExecCommand(card, dbReq, dbResp,
                          LC_Client_GetShortTimeout(LC_Card_GetClient(card)));
  if (res!=LC_Client_ResultOk) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    return res;
  }

  GWEN_DB_Group_free(dbResp);
  GWEN_DB_Group_free(dbReq);
  return res;

}









