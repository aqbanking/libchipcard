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


#include "card_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/gwentime.h>
#include <chipcard2/chipcard2.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LC_CARD, LC_Card)
GWEN_INHERIT_FUNCTIONS(LC_CARD)


LC_CARD *LC_Card_new(LC_CLIENT *cl,
                     GWEN_TYPE_UINT32 cardId,
                     GWEN_TYPE_UINT32 serverId,
                     const char *cardType,
                     GWEN_TYPE_UINT32 rflags,
                     GWEN_BUFFER *atr){
  LC_CARD *cd;

  assert(cl);
  assert(cardId);
  assert(serverId);
  assert(cardType);

  GWEN_NEW_OBJECT(LC_CARD, cd);
  GWEN_LIST_INIT(LC_CARD, cd);
  GWEN_INHERIT_INIT(LC_CARD, cd);
  cd->client=cl;
  cd->cardId=cardId;
  cd->serverId=serverId;
  cd->cardType=strdup(cardType);
  cd->readerFlags=rflags;
  if (atr)
    cd->atr=atr;

  cd->openFn=LC_Card__Open;
  cd->closeFn=LC_Card__Close;

  cd->driverVars=GWEN_DB_Group_new("driverVars");

  return cd;
}



void LC_Card_free(LC_CARD *cd){
  if (cd) {
    GWEN_INHERIT_FINI(LC_CARD, cd);
    GWEN_DB_Group_free(cd->driverVars);
    free(cd->cardType);
    free(cd->lastResult);
    free(cd->lastText);
    GWEN_Buffer_free(cd->atr);
    LC_CardContext_free(cd->context);
    GWEN_LIST_FINI(LC_CARD, cd);
    GWEN_FREE_OBJECT(cd);
  }
}



void LC_Card_SetLastResult(LC_CARD *cd,
                           const char *result,
                           const char *text,
                           int sw1, int sw2){
  assert(cd);
  free(cd->lastResult);
  free(cd->lastText);
  if (result)
    cd->lastResult=strdup(result);
  else
    cd->lastResult=0;
  if (text)
    cd->lastText=strdup(text);
  else
    cd->lastText=0;
  cd->lastSW1=sw1;
  cd->lastSW2=sw2;
}


int LC_Card_GetLastSW1(const LC_CARD *cd){
  assert(cd);
  return cd->lastSW1;
}



int LC_Card_GetLastSW2(const LC_CARD *cd){
  assert(cd);
  return cd->lastSW2;
}



const char *LC_Card_GetLastResult(const LC_CARD *cd){
  assert(cd);
  return cd->lastResult;
}



const char *LC_Card_GetLastText(const LC_CARD *cd){
  assert(cd);
  return cd->lastText;
}



LC_CLIENT *LC_Card_GetClient(const LC_CARD *cd){
  assert(cd);
  return cd->client;
}



GWEN_TYPE_UINT32 LC_Card_GetCardId(const LC_CARD *cd){
  assert(cd);
  return cd->cardId;
}



GWEN_TYPE_UINT32 LC_Card_GetServerId(const LC_CARD *cd){
  assert(cd);
  return cd->serverId;
}



const char *LC_Card_GetReaderType(const LC_CARD *cd){
  assert(cd);
  return cd->readerType;
}



const char *LC_Card_GetDriverType(const LC_CARD *cd){
  assert(cd);
  return cd->driverType;
}



GWEN_TYPE_UINT32 LC_Card_GetReaderFlags(const LC_CARD *cd){
  assert(cd);
  return cd->readerFlags;
}



const char *LC_Card_GetCardType(const LC_CARD *cd){
  assert(cd);
  return cd->cardType;
}



void LC_Card_SetCardType(LC_CARD *cd, const char *ct){
  assert(cd);
  assert(ct);

  free(cd->cardType);
  cd->cardType=strdup(ct);
}



GWEN_BUFFER *LC_Card_GetAtr(const LC_CARD *cd){
  assert(cd);
  return cd->atr;
}



LC_CARDCONTEXT *LC_Card_GetContext(const LC_CARD *cd){
  assert(cd);
  return cd->context;
}



void LC_Card_SetContext(LC_CARD *cd, LC_CARDCONTEXT *ctx){
  assert(cd);
  if (cd->context)
    LC_CardContext_free(cd->context);
  cd->context=ctx;
}



void LC_Card_Dump(const LC_CARD *cd, FILE *f, int insert) {
  int k;

  for (k=0; k<insert; k++)
    fprintf(f, " ");
  fprintf(f, "Card\n");
  for (k=0; k<insert; k++)
    fprintf(f, " ");
  fprintf(f,
          "==================="
          "==================="
          "==================="
          "==================\n");
  for (k=0; k<insert; k++)
    fprintf(f, " ");
  fprintf(f, "Card id     : %08x\n", cd->cardId);
  for (k=0; k<insert; k++)
    fprintf(f, " ");
  fprintf(f, "Card type   : %s\n", cd->cardType);
  for (k=0; k<insert; k++)
    fprintf(f, " ");
  fprintf(f, "Server id   : %08x\n", cd->serverId);
  for (k=0; k<insert; k++)
    fprintf(f, " ");
  fprintf(f, "Reader flags:");
  if (cd->readerFlags & LC_CARD_READERFLAGS_KEYPAD)
    fprintf(f, " keypad");
  if (cd->readerFlags & LC_CARD_READERFLAGS_DISPLAY)
    fprintf(f, " display");
  fprintf(f, "\n");
  if (cd->atr) {
    for (k=0; k<insert; k++)
      fprintf(f, " ");
    fprintf(f, "ATR\n");
    for (k=0; k<insert; k++)
      fprintf(f, " ");
    fprintf(f,
            "-------------------"
            "-------------------"
            "-------------------"
            "------------------\n");
    GWEN_Text_DumpString(GWEN_Buffer_GetStart(cd->atr),
                         GWEN_Buffer_GetUsedBytes(cd->atr),
                         f,
                         insert+2);
  }
  for (k=0; k<insert; k++)
    fprintf(f, " ");
  fprintf(f,
          "==================="
          "==================="
          "==================="
          "==================\n");
}



LC_CLIENT_RESULT LC_Card_Open(LC_CARD *card) {
  assert(card);
  if (!card->openFn) {
    DBG_ERROR(LC_LOGDOMAIN, "No OpenFn set");
    return LC_Client_ResultCmdError;
  }
  return card->openFn(card);
}



LC_CLIENT_RESULT LC_Card_Close(LC_CARD *card) {
  assert(card);
  if (!card->closeFn) {
    DBG_ERROR(LC_LOGDOMAIN, "No CloseFn set");
    return LC_Client_ResultCmdError;
  }
  return card->closeFn(card);
}



LC_CLIENT_RESULT LC_Card__Open(LC_CARD *card) {
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_Client_SendTakeCard(card->client, card);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"takeCard\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_Client_CheckResponse_Wait(card->client, rqid,
                                   LC_Client_GetShortTimeout(card->client));
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_Client_DeleteRequest(card->client, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"takeCard\"");
    }
    return res;
  }
  res=LC_Client_CheckTakeCard(card->client, rqid);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"takeCard\"");
    return LC_Client_ResultCmdError;
  }

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_Card__Close(LC_CARD *card) {
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_Client_SendReleaseCard(card->client, card);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"releaseCard\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_Client_CheckResponse_Wait(card->client, rqid,
                                   LC_Client_GetShortTimeout(card->client));
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_Client_DeleteRequest(card->client, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"releaseCard\"");
    }
    return res;
  }
  res=LC_Client_CheckReleaseCard(card->client, rqid);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"releaseCard\"");
    return LC_Client_ResultCmdError;
  }

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_Card_SelectCardAndApp(LC_CARD *card,
                                          const char *cardName,
                                          const char *appName){
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_Client_SendSelectCardApp(card->client, card, cardName, appName);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"selectCardApp\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_Client_CheckResponse_Wait(card->client, rqid,
                                   LC_Client_GetShortTimeout(card->client));
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_Client_DeleteRequest(card->client, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"selectCardApp\"");
    }
    return res;
  }
  res=LC_Client_CheckSelectCardApp(card->client, rqid);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"selectCardApp\"");
    return LC_Client_ResultCmdError;
  }

  return LC_Client_ResultOk;
}


LC_CLIENT_RESULT LC_Card_ExecCommand(LC_CARD *card,
                                     GWEN_DB_NODE *dbReq,
                                     GWEN_DB_NODE *dbRsp,
                                     int timeout) {
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_Client_SendExecCommand(card->client, card, dbReq);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"execCommand\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_Client_CheckResponse_Wait(card->client, rqid, timeout);
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_Client_DeleteRequest(card->client, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"execCommand\"");
    }
    return res;
  }
  res=LC_Client_CheckExecCommand(card->client, rqid, dbRsp);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"execCommand\"");
    return LC_Client_ResultCmdError;
  }

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_Card_ExecAPDU(LC_CARD *card,
                                  const char *apdu,
                                  unsigned int len,
                                  GWEN_BUFFER *rbuf,
                                  LC_CLIENT_CMDTARGET t,
                                  int timeout){
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_Client_SendCommandCard(card->client, card, apdu, len, t);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"commandCard\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_Client_CheckResponse_Wait(card->client, rqid,
                                   timeout);
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_Client_DeleteRequest(card->client, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"commandCard\"");
    }
    return res;
  }
  res=LC_Client_CheckCommandCard(card->client, rqid, rbuf);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"commandCard\"");
    return LC_Client_ResultCmdError;
  }

  return LC_Client_ResultOk;
}



int LC_Card_GetRecordNumber(LC_CARD *card, const char *recName){
  assert(card);

  if (!card->context) {
    DBG_ERROR(LC_LOGDOMAIN, "No card/application selected");
    return -1;
  }
  return LC_CardContext_GetRecordNumber(card->context, recName);
}



int LC_Card_ParseRecord(LC_CARD *card,
                        int recNum,
                        GWEN_BUFFER *buf,
                        GWEN_DB_NODE *dbRecord){
  assert(card);

  if (!card->context) {
    DBG_ERROR(LC_LOGDOMAIN, "No card/application selected");
    return -1;
  }
  return LC_CardContext_ParseRecord(card->context, recNum, buf, dbRecord);
}



int LC_Card_CreateRecord(LC_CARD *card,
                         int recNum,
                         GWEN_BUFFER *buf,
                         GWEN_DB_NODE *dbRecord){
  assert(card);

  if (!card->context) {
    DBG_ERROR(LC_LOGDOMAIN, "No card/application selected");
    return -1;
  }
  return LC_CardContext_CreateRecord(card->context, recNum, buf, dbRecord);
}



LC_CARD_OPEN_FN LC_Card_GetOpenFn(const LC_CARD *card){
  assert(card);
  return card->openFn;
}



void LC_Card_SetOpenFn(LC_CARD *card, LC_CARD_OPEN_FN fn){
  assert(card);
  card->openFn=fn;
}



LC_CARD_CLOSE_FN LC_Card_GetCloseFn(const LC_CARD *card){
  assert(card);
  return card->closeFn;
}



void LC_Card_SetCloseFn(LC_CARD *card, LC_CARD_CLOSE_FN fn){
  assert(card);
  card->closeFn=fn;
}



LC_CLIENT_RESULT LC_Card_ReadBinary(LC_CARD *card,
                                    int offset,
                                    int size,
                                    GWEN_BUFFER *buf){
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;
  unsigned int bs;
  const void *p;

  DBG_INFO(LC_LOGDOMAIN, "Reading binary %04x:%04x", offset, size);

  dbReq=GWEN_DB_Group_new("ReadBinary");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "offset", offset);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "lr", size);

  res=LC_Card_ExecCommand(card, dbReq, dbResp,
                          LC_Client_GetLongTimeout(LC_Card_GetClient(card)));
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



LC_CLIENT_RESULT LC_Card_WriteBinary(LC_CARD *card,
                                     int offset,
                                     GWEN_BUFFER *buf){
  LC_CLIENT_RESULT res;

  res=LC_Card_WriteBinary2(card, offset,
                           GWEN_Buffer_GetStart(buf),
                           GWEN_Buffer_GetUsedBytes(buf));
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  return res;
}



LC_CLIENT_RESULT LC_Card_WriteBinary2(LC_CARD *card,
                                      int offset,
                                      const char *ptr,
                                      unsigned int size){
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;

  DBG_ERROR(LC_LOGDOMAIN, "Writing binary %04x:%04x", offset, size);

  dbReq=GWEN_DB_Group_new("WriteBinary");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "offset", offset);
  if (ptr) {
    if (size) {
      GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                          "data", ptr, size);
    }
  }

  res=LC_Card_ExecCommand(card, dbReq, dbResp,
                          LC_Client_GetLongTimeout(LC_Card_GetClient(card)));
  if (res!=LC_Client_ResultOk) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    return res;
  }

  /* successful */
  GWEN_DB_Group_free(dbResp);
  GWEN_DB_Group_free(dbReq);
  return res;
}



int LC_Card_ParseData(LC_CARD *card,
                      const char *format,
                      GWEN_BUFFER *buf,
                      GWEN_DB_NODE *dbData){
  assert(card);

  if (!card->context) {
    DBG_ERROR(LC_LOGDOMAIN, "No card/application selected");
    return -1;
  }
  return LC_CardContext_ParseData(card->context, format, buf, dbData);
}



int LC_Card_CreateData(LC_CARD *card,
                       const char *format,
                       GWEN_BUFFER *buf,
                       GWEN_DB_NODE *dbData){
  assert(card);

  if (!card->context) {
    DBG_ERROR(LC_LOGDOMAIN, "No card/application selected");
    return -1;
  }
  return LC_CardContext_CreateData(card->context, format, buf, dbData);
}



LC_CLIENT_RESULT LC_Card_SelectMF(LC_CARD *card){
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;

  dbReq=GWEN_DB_Group_new("SelectMF");
  dbResp=GWEN_DB_Group_new("response");
  res=LC_Card_ExecCommand(card, dbReq, dbResp,
                          LC_Client_GetShortTimeout(LC_Card_GetClient(card)));
  GWEN_DB_Group_free(dbReq);
  GWEN_DB_Group_free(dbResp);
  return res;
}



GWEN_XMLNODE *LC_Card_GetEfInfo(const LC_CARD *card){
  GWEN_XMLNODE *n;

  assert(card);

  if (!card->context) {
    DBG_ERROR(LC_LOGDOMAIN, "No card/application selected");
    return 0;
  }
  n=LC_CardContext_GetEfNode(card->context);
  if (n)
    return GWEN_XMLNode_dup(n);
  return 0;
}



GWEN_XMLNODE *LC_Card_GetDfInfo(const LC_CARD *card){
  GWEN_XMLNODE *n;

  assert(card);

  if (!card->context) {
    DBG_ERROR(LC_LOGDOMAIN, "No card/application selected");
    return 0;
  }
  n=LC_CardContext_GetDfNode(card->context);
  if (n)
    return GWEN_XMLNode_dup(n);
  return 0;
}



LC_CLIENT_RESULT LC_Card_GetDriverVar(LC_CARD *card,
                                      const char *varName,
                                      GWEN_BUFFER *vbuf) {
  LC_CLIENT_RESULT res;
  const char *value;

  value=GWEN_DB_GetCharValue(card->driverVars, varName, 0, 0);
  if (value) {
    GWEN_Buffer_AppendString(vbuf, value);
    return LC_Client_ResultOk;
  }

  res=LC_Client_GetDriverVar(card->client,
                             card,
                             varName,
                             vbuf);

  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  if (GWEN_Buffer_GetUsedBytes(vbuf))
    GWEN_DB_SetCharValue(card->driverVars, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         varName, GWEN_Buffer_GetStart(vbuf));

  return LC_Client_ResultOk;
}















