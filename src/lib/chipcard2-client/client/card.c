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
#include "client_l.h"
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
GWEN_LIST2_FUNCTIONS(LC_CARD, LC_Card)


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
  cd->cardTypes=GWEN_StringList_new();

  return cd;
}



void LC_Card_free(LC_CARD *cd){
  if (cd) {
    GWEN_INHERIT_FINI(LC_CARD, cd);
    GWEN_DB_Group_free(cd->driverVars);
    free(cd->selectedApp);
    free(cd->cardType);
    free(cd->lastResult);
    free(cd->lastText);
    GWEN_StringList_free(cd->cardTypes);
    GWEN_Buffer_free(cd->atr);
    LC_CardContext_free(cd->context);
    GWEN_LIST_FINI(LC_CARD, cd);
    GWEN_FREE_OBJECT(cd);
  }
}



const GWEN_STRINGLIST *LC_Card_GetCardTypes(const LC_CARD *cd){
  assert(cd);
  return cd->cardTypes;
}



int LC_Card_AddCardType(LC_CARD *cd, const char *s) {
  assert(cd);
  return GWEN_StringList_AppendString(cd->cardTypes, s, 0, 1);
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



void LC_Card_ResetCardId(LC_CARD *cd){
  assert(cd);
  cd->cardId=0;
}



void LC_Card_Dump(const LC_CARD *cd, FILE *f, int insert) {
  int k;
  GWEN_STRINGLISTENTRY *se;

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
  fprintf(f, "Card types  :");
  se=GWEN_StringList_FirstEntry(cd->cardTypes);
  while(se) {
    const char *s;

    s=GWEN_StringListEntry_Data(se);
    assert(s);
    fprintf(f, " %s", s);
    se=GWEN_StringListEntry_Next(se);
  } /* while */
  fprintf(f, "\n");
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
  if (cd->readerFlags & LC_CARD_READERFLAGS_NOINFO)
    fprintf(f, " noinfo");
  if (cd->readerFlags & LC_CARD_READERFLAGS_REMOTE)
    fprintf(f, " remote");
  if (cd->readerFlags & LC_CARD_READERFLAGS_AUTO)
    fprintf(f, " auto");
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
  LC_CLIENT_RESULT res;

  assert(card);
  if (!card->closeFn) {
    DBG_ERROR(LC_LOGDOMAIN, "No CloseFn set");
    res=LC_Client_ResultCmdError;
  }
  else
    res=card->closeFn(card);

  free(card->selectedApp);
  card->selectedApp=0;
  return res;
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

  free(card->selectedApp);
  card->selectedApp=0;

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

  free(card->selectedApp);
  card->selectedApp=strdup(appName);

  return LC_Client_ResultOk;
}



int LC_Card_SelectApp(LC_CARD *card, const char *appName){
  int rv;

  assert(card);
  assert(appName);
  rv=LC_Client_SelectApp(card->client, card, appName);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return rv;
  }
  free(card->selectedApp);
  card->selectedApp=strdup(appName);
  return 0;
}



const char *LC_Card_GetSelectedApp(const LC_CARD *card) {
  assert(card);
  return card->selectedApp;
}



void LC_Card_ShowError(LC_CARD *card, LC_CLIENT_RESULT res,
                       const char *failedCommand) {
  const char *s;

  switch(res) {
  case LC_Client_ResultOk:
    s="Ok.";
    break;
  case LC_Client_ResultWait:
    s="Timeout.";
    break;
  case LC_Client_ResultIpcError:
    s="IPC error.";
    break;
  case LC_Client_ResultCmdError:
    s="Command error.";
    break;
  case LC_Client_ResultDataError:
    s="Data error.";
    break;
  case LC_Client_ResultAborted:
    s="Aborted.";
    break;
  case LC_Client_ResultInvalid:
    s="Invalid argument to command.";
    break;
  case LC_Client_ResultInternal:
    s="Internal error.";
    break;
  case LC_Client_ResultGeneric:
    s="Generic error.";
    break;
  default:
    s="Unknown error.";
    break;
  }

  if (res==LC_Client_ResultCmdError) {
    int sw1;
    int sw2;

    sw1=LC_Card_GetLastSW1(card);
    sw2=LC_Card_GetLastSW2(card);
    if (sw1!=-1 && sw2!=-1) {
      DBG_INFO(LC_LOGDOMAIN,
               "Error in \"%s\": %s (SW1=%02x, SW2=%02x)\n",
               failedCommand, s, sw1, sw2);
    }
    else {
      DBG_INFO(LC_LOGDOMAIN,
               "Error in \"%s\": %s\n",
               failedCommand, s);
    }
    s=LC_Card_GetLastResult(card);
    if (s) {
      DBG_INFO(LC_LOGDOMAIN, "- Result: %s\n", s);
    }
    s=LC_Card_GetLastText(card);
    if (s) {
      DBG_INFO(LC_LOGDOMAIN, "- Text: %s\n", s);
    }
  }
  else {
    DBG_INFO(LC_LOGDOMAIN,
             "Error in \"%s\": %s\n", failedCommand, s);
  }
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
      DBG_NOTICE(LC_LOGDOMAIN, "User aborted");
      LC_Client_DeleteRequest(card->client, rqid);
    }
    else {
      DBG_INFO(LC_LOGDOMAIN, "No response for request \"execCommand\"");
    }
    return res;
  }
  res=LC_Client_CheckExecCommand(card->client, rqid, dbRsp);
  if (res!=LC_Client_ResultOk) {
    GWEN_BUFFER *cbuf;

    cbuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_AppendString(cbuf, "execCommand(");
    GWEN_Buffer_AppendString(cbuf, GWEN_DB_GroupName(dbReq));
    GWEN_Buffer_AppendString(cbuf, ")");

    LC_Card_ShowError(card, res, GWEN_Buffer_GetStart(cbuf));
    GWEN_Buffer_free(cbuf);
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



LC_CLIENT_RESULT LC_Card_Check(LC_CARD *card){
  return LC_Client_CardCheck(card->client, card);
}



LC_CLIENT_RESULT LC_Card_Reset(LC_CARD *card){
  return LC_Client_CardReset(card->client, card);
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



void LC_Card_SetGetPinStatusFn(LC_CARD *card, LC_CARD_GETPINSTATUS_FN fn) {
  assert(card);
  card->getPinStatusFn=fn;
}



void LC_Card_SetGetInitialPinFn(LC_CARD *card, LC_CARD_GETINITIALPIN_FN fn){
  assert(card);
  card->getInitialPinFn=fn;
}



LC_CLIENT_RESULT LC_Card_ReadBinary(LC_CARD *card,
                                    int offset,
                                    int size,
                                    GWEN_BUFFER *buf){
  return LC_Card_IsoReadBinary(card, 0, offset, size, buf);
}



LC_CLIENT_RESULT LC_Card_WriteBinary(LC_CARD *card,
                                     int offset,
                                     GWEN_BUFFER *buf){
  return LC_Card_IsoUpdateBinary(card, 0, offset,
                                 GWEN_Buffer_GetStart(buf),
                                 GWEN_Buffer_GetUsedBytes(buf));
}



LC_CLIENT_RESULT LC_Card_WriteBinary2(LC_CARD *card,
                                      int offset,
                                      const char *ptr,
                                      unsigned int size){
  return LC_Card_IsoUpdateBinary(card, 0, offset, ptr, size);
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



GWEN_XMLNODE *LC_Card__GetEfInfo(const LC_CARD *card){
  assert(card);

  if (!card->context) {
    DBG_ERROR(LC_LOGDOMAIN, "No card/application selected");
    return 0;
  }
  return LC_CardContext_GetEfNode(card->context);
}



GWEN_XMLNODE *LC_Card_GetEfInfo(const LC_CARD *card){
  GWEN_XMLNODE *n;

  n=LC_Card__GetEfInfo(card);
  if (n)
    return GWEN_XMLNode_dup(n);
  return 0;
}



GWEN_XMLNODE *LC_Card__GetDfInfo(const LC_CARD *card){
  assert(card);

  if (!card->context) {
    DBG_ERROR(LC_LOGDOMAIN, "No card/application selected");
    return 0;
  }
  return LC_CardContext_GetDfNode(card->context);
}



GWEN_XMLNODE *LC_Card_GetDfInfo(const LC_CARD *card){
  GWEN_XMLNODE *n;

  n=LC_Card__GetDfInfo(card);
  if (n)
    return GWEN_XMLNode_dup(n);
  return 0;
}



GWEN_XMLNODE *LC_Card__GetAppInfo(const LC_CARD *card){
  assert(card);

  if (!card->context) {
    DBG_ERROR(LC_LOGDOMAIN, "No card/application selected");
    return 0;
  }
  return LC_CardContext_GetAppNode(card->context);
}



GWEN_XMLNODE *LC_Card_GetAppInfo(const LC_CARD *card){
  GWEN_XMLNODE *n;

  n=LC_Card__GetAppInfo(card);
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



void LC_Card_List2_freeAll(LC_CARD_LIST2 *l){
  if (l) {
    LC_CARD_LIST2_ITERATOR *cit;

    cit=LC_Card_List2_First(l);
    if (cit) {
      LC_CARD *card;

      card=LC_Card_List2Iterator_Data(cit);
      while(card) {
        LC_CARD *next;

        next=LC_Card_List2Iterator_Next(cit);
        LC_Card_free(card);
        card=next;
      } /* while */
      LC_Card_List2Iterator_free(cit);
    }
    LC_Card_List2_free(l);
  }
}






/* __________________________________________________________________________
 * AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
 *                             ISO commands
 * YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY
 */



void LC_Card_SetIsoReadBinaryFn(LC_CARD *card, LC_CARD_ISOREADBINARY_FN f) {
  assert(card);
  card->readBinaryFn=f;
}



void LC_Card_SetIsoWriteBinaryFn(LC_CARD *card, LC_CARD_ISOWRITEBINARY_FN f){
  assert(card);
  card->writeBinaryFn=f;
}



void LC_Card_SetIsoUpdateBinaryFn(LC_CARD *card,
                                  LC_CARD_ISOUPDATEBINARY_FN f){
  assert(card);
  card->updateBinaryFn=f;
}



void LC_Card_SetIsoEraseBinaryFn(LC_CARD *card, LC_CARD_ISOERASEBINARY_FN f) {
  assert(card);
  card->eraseBinaryFn=f;
}



void LC_Card_SetIsoReadRecordFn(LC_CARD *card, LC_CARD_ISOREADRECORD_FN f){
  assert(card);
  card->readRecordFn=f;
}



void LC_Card_SetIsoWriteRecordFn(LC_CARD *card, LC_CARD_ISOWRITERECORD_FN f){
  assert(card);
  card->writeRecordFn=f;
}



void LC_Card_SetIsoUpdateRecordFn(LC_CARD *card,
                                  LC_CARD_ISOUPDATERECORD_FN f) {
  assert(card);
  card->updateRecordFn=f;
}



void LC_Card_SetIsoAppendRecordFn(LC_CARD *card,
                                  LC_CARD_ISOAPPENDRECORD_FN f){
  assert(card);
  card->appendRecordFn=f;
}



void LC_Card_SetIsoVerifyPinFn(LC_CARD *card, LC_CARD_ISOVERIFYPIN_FN f) {
  assert(card);
  card->verifyPinFn=f;
}



void LC_Card_SetIsoModifyPinFn(LC_CARD *card, LC_CARD_ISOMODIFYPIN_FN f) {
  assert(card);
  card->modifyPinFn=f;
}



void LC_Card_SetIsoPerformVerificationFn(LC_CARD *card,
                                         LC_CARD_ISOPERFORMVERIFICATION_FN f){
  assert(card);
  card->performVerificationFn=f;
}



void LC_Card_SetIsoPerformModificationFn(LC_CARD *card,
                                         LC_CARD_ISOPERFORMMODIFICATION_FN f){
  assert(card);
  card->performModificationFn=f;
}



void LC_Card_SetIsoManageSeFn(LC_CARD *card, LC_CARD_ISOMANAGESE_FN f) {
  assert(card);
  card->manageSeFn=f;
}



void LC_Card_SetIsoSignFn(LC_CARD *card, LC_CARD_ISOSIGN_FN f) {
  assert(card);
  card->signFn=f;
}



void LC_Card_SetIsoVerifyFn(LC_CARD *card, LC_CARD_ISOVERIFY_FN f) {
  assert(card);
  card->verifyFn=f;
}



void LC_Card_SetIsoEncipherFn(LC_CARD *card, LC_CARD_ISOENCIPHER_FN f) {
  assert(card);
  card->encipherFn=f;
}



void LC_Card_SetIsoDecipherFn(LC_CARD *card, LC_CARD_ISODECIPHER_FN f) {
  assert(card);
  card->decipherFn=f;
}






LC_CLIENT_RESULT LC_Card_IsoReadBinary(LC_CARD *card,
                                       GWEN_TYPE_UINT32 flags,
				       int offset,
                                       int size,
                                       GWEN_BUFFER *buf) {
  assert(card);
  if (card->readBinaryFn)
    return card->readBinaryFn(card, flags, offset, size, buf);
  else
    return LC_Card__IsoReadBinary(card, flags, offset, size, buf);
}



LC_CLIENT_RESULT LC_Card_IsoWriteBinary(LC_CARD *card,
					GWEN_TYPE_UINT32 flags,
					int offset,
					const char *ptr,
                                        unsigned int size) {
  assert(card);
  if (card->writeBinaryFn)
    return card->writeBinaryFn(card, flags, offset, ptr, size);
  else
    return LC_Card__IsoWriteBinary(card, flags, offset, ptr, size);
}



LC_CLIENT_RESULT LC_Card_IsoUpdateBinary(LC_CARD *card,
					 GWEN_TYPE_UINT32 flags,
					 int offset,
					 const char *ptr,
                                         unsigned int size) {
  assert(card);
  if (card->updateBinaryFn)
    return card->updateBinaryFn(card, flags, offset, ptr, size);
  else
    return LC_Card__IsoUpdateBinary(card, flags, offset, ptr, size);
}




LC_CLIENT_RESULT LC_Card_IsoEraseBinary(LC_CARD *card,
					GWEN_TYPE_UINT32 flags,
					int offset,
                                        unsigned int size) {
  assert(card);
  if (card->eraseBinaryFn)
    return card->eraseBinaryFn(card, flags, offset, size);
  else
    return LC_Card__IsoEraseBinary(card, flags, offset, size);
}



LC_CLIENT_RESULT LC_Card_IsoReadRecord(LC_CARD *card,
				       GWEN_TYPE_UINT32 flags,
				       int recNum,
                                       GWEN_BUFFER *buf) {
  assert(card);
  if (card->readRecordFn)
    return card->readRecordFn(card, flags, recNum, buf);
  else
    return LC_Card__IsoReadRecord(card, flags, recNum, buf);
}



LC_CLIENT_RESULT LC_Card_IsoWriteRecord(LC_CARD *card,
					GWEN_TYPE_UINT32 flags,
					int recNum,
					const char *ptr,
                                        unsigned int size) {
  assert(card);
  if (card->writeRecordFn)
    return card->writeRecordFn(card, flags, recNum, ptr, size);
  else
    return LC_Card__IsoWriteRecord(card, flags, recNum, ptr, size);
}



LC_CLIENT_RESULT LC_Card_IsoAppendRecord(LC_CARD *card,
                                         GWEN_TYPE_UINT32 flags,
                                         const char *ptr,
                                         unsigned int size) {
  assert(card);
  if (card->appendRecordFn)
    return card->appendRecordFn(card, flags, ptr, size);
  else
    return LC_Card__IsoAppendRecord(card, flags, ptr, size);
}




LC_CLIENT_RESULT LC_Card_IsoUpdateRecord(LC_CARD *card,
					 GWEN_TYPE_UINT32 flags,
					 int recNum,
					 const char *ptr,
                                         unsigned int size) {
  assert(card);
  if (card->updateRecordFn)
    return card->updateRecordFn(card, flags, recNum, ptr, size);
  else
    return LC_Card__IsoUpdateRecord(card, flags, recNum, ptr, size);
}



LC_CLIENT_RESULT LC_Card_IsoVerifyPin(LC_CARD *card,
                                      GWEN_TYPE_UINT32 flags,
                                      const LC_PININFO *pi,
                                      const unsigned char *ptr,
                                      unsigned int size,
                                      int *triesLeft) {
  assert(card);
  if (card->verifyPinFn)
    return card->verifyPinFn(card, flags, pi, ptr, size,
			     triesLeft);
  else
    return LC_Card__IsoVerifyPin(card, flags, pi, ptr, size,
                                 triesLeft);
}



LC_CLIENT_RESULT LC_Card_IsoModifyPin(LC_CARD *card,
                                      GWEN_TYPE_UINT32 flags,
                                      const LC_PININFO *pi,
                                      const unsigned char *oldptr,
                                      unsigned int oldsize,
                                      const unsigned char *newptr,
                                      unsigned int newsize,
                                      int *triesLeft) {
  assert(card);
  if (card->modifyPinFn)
    return card->modifyPinFn(card, flags, pi,
                             oldptr, oldsize,
                             newptr, newsize,
                             triesLeft);
  else
    return LC_Card__IsoModifyPin(card, flags, pi,
				 oldptr, oldsize,
                                 newptr, newsize,
                                 triesLeft);
}



LC_CLIENT_RESULT LC_Card_IsoPerformVerification(LC_CARD *card,
                                                GWEN_TYPE_UINT32 flags,
                                                const LC_PININFO *pi,
                                                int *triesLeft) {
  assert(card);
  if (card->performVerificationFn)
    return card->performVerificationFn(card, flags, pi,
                                       triesLeft);
  else
    return LC_Card__IsoPerformVerification(card, flags, pi,
                                           triesLeft);
}



LC_CLIENT_RESULT LC_Card_IsoPerformModification(LC_CARD *card,
                                                GWEN_TYPE_UINT32 flags,
                                                const LC_PININFO *pi,
                                                int *triesLeft) {
  assert(card);
  if (card->performModificationFn)
    return card->performModificationFn(card, flags, pi,
                                       triesLeft);
  else
    return LC_Card__IsoPerformModification(card, flags, pi,
                                           triesLeft);
}



LC_CLIENT_RESULT LC_Card_IsoManageSe(LC_CARD *card,
                                     int tmpl, int kids, int kidp, int ar) {
  assert(card);
  if (card->manageSeFn)
    return card->manageSeFn(card, tmpl, kids, kidp, ar);
  else
    return LC_Card__IsoManageSe(card, tmpl, kids, kidp, ar);
}



LC_CLIENT_RESULT LC_Card_IsoEncipher(LC_CARD *card,
                                     const char *ptr,
                                     unsigned int size,
                                     GWEN_BUFFER *codeBuf) {
  assert(card);
  if (card->encipherFn)
    return card->encipherFn(card, ptr, size, codeBuf);
  else
    return LC_Card__IsoEncipher(card, ptr, size, codeBuf);
}



LC_CLIENT_RESULT LC_Card_IsoDecipher(LC_CARD *card,
                                     const char *ptr,
                                     unsigned int size,
                                     GWEN_BUFFER *plainBuf) {
  assert(card);
  if (card->decipherFn)
    return card->decipherFn(card, ptr, size, plainBuf);
  else
    return LC_Card__IsoDecipher(card, ptr, size, plainBuf);
}



LC_CLIENT_RESULT LC_Card_IsoSign(LC_CARD *card,
                                 const char *ptr,
                                 unsigned int size,
                                 GWEN_BUFFER *sigBuf) {
  assert(card);
  if (card->signFn)
    return card->signFn(card, ptr, size, sigBuf);
  else
    return LC_Client_ResultNotSupported;
}



LC_CLIENT_RESULT LC_Card_IsoVerify(LC_CARD *card,
                                   const char *dptr,
                                   unsigned int dsize,
                                   const char *sigptr,
                                   unsigned int sigsize) {
  assert(card);
  if (card->verifyFn)
    return card->verifyFn(card, dptr, dsize, sigptr, sigsize);
  else
    return LC_Client_ResultNotSupported;
}


















LC_CLIENT_RESULT LC_Card__IsoReadBinary(LC_CARD *card,
                                        GWEN_TYPE_UINT32 flags,
                                        int offset,
                                        int size,
                                        GWEN_BUFFER *buf){
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;
  unsigned int bs;
  const void *p;

  DBG_INFO(LC_LOGDOMAIN, "Reading binary %04x:%04x", offset, size);

  if (flags & LC_CARD_ISO_FLAGS_EFID_MASK) {
    if (offset>255) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Offset too high when implicitly selecting EF "
                "("GWEN_TYPE_TMPL_UINT32")", flags);
      return LC_Client_ResultInvalid;
    }
    /* modify offset: highbyte is p1, lowbyte is p2 */
    offset|=0x8000;
    offset|=((flags & LC_CARD_ISO_FLAGS_EFID_MASK)<<8);
  }

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



LC_CLIENT_RESULT LC_Card__IsoUpdateBinary(LC_CARD *card,
                                          GWEN_TYPE_UINT32 flags,
                                          int offset,
                                          const char *ptr,
                                          unsigned int size){
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;

  DBG_ERROR(LC_LOGDOMAIN, "Writing binary %04x:%04x", offset, size);

  if (flags & LC_CARD_ISO_FLAGS_EFID_MASK) {
    if (offset>255) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Offset too high when implicitly selecting EF "
                "("GWEN_TYPE_TMPL_UINT32")", flags);
      return LC_Client_ResultInvalid;
    }
    /* modify offset: highbyte is p1, lowbyte is p2 */
    offset|=0x8000;
    offset|=((flags & LC_CARD_ISO_FLAGS_EFID_MASK)<<8);
  }

  dbReq=GWEN_DB_Group_new("UpdateBinary");
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



LC_CLIENT_RESULT LC_Card__IsoWriteBinary(LC_CARD *card,
                                         GWEN_TYPE_UINT32 flags,
                                         int offset,
                                         const char *ptr,
                                         unsigned int size){
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;

  DBG_ERROR(LC_LOGDOMAIN, "Writing binary %04x:%04x", offset, size);

  if (flags & LC_CARD_ISO_FLAGS_EFID_MASK) {
    if (offset>255) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Offset too high when implicitly selecting EF "
                "("GWEN_TYPE_TMPL_UINT32")", flags);
      return LC_Client_ResultInvalid;
    }
    /* modify offset: highbyte is p1, lowbyte is p2 */
    offset|=0x8000;
    offset|=((flags & LC_CARD_ISO_FLAGS_EFID_MASK)<<8);
  }

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



LC_CLIENT_RESULT LC_Card__IsoEraseBinary(LC_CARD *card,
                                         GWEN_TYPE_UINT32 flags,
                                         int offset,
                                         unsigned int size){
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;

  DBG_ERROR(LC_LOGDOMAIN, "Erasing binary %04x:%04x", offset, size);

  if (flags & LC_CARD_ISO_FLAGS_EFID_MASK) {
    if (offset>255) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Offset too high when implicitly selecting EF "
                "("GWEN_TYPE_TMPL_UINT32")", flags);
      return LC_Client_ResultInvalid;
    }
    /* modify offset: highbyte is p1, lowbyte is p2 */
    offset|=0x8000;
    offset|=((flags & LC_CARD_ISO_FLAGS_EFID_MASK)<<8);
  }

  dbReq=GWEN_DB_Group_new("EraseBinary");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "offset", offset);
  if (size!=0)
    GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                        "len", size);

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



LC_CLIENT_RESULT LC_Card__IsoReadRecord(LC_CARD *card,
                                        GWEN_TYPE_UINT32 flags,
                                        int recNum,
                                        GWEN_BUFFER *buf){
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;
  unsigned int bs;
  const void *p;
  unsigned char p2;

  p2=(flags & LC_CARD_ISO_FLAGS_EFID_MASK)<<3;
  if ((flags & LC_CARD_ISO_FLAGS_RECSEL_MASK)!=
      LC_CARD_ISO_FLAGS_RECSEL_GIVEN) {
    DBG_ERROR(LC_LOGDOMAIN,
              "Invalid flags "GWEN_TYPE_TMPL_UINT32
              " (only RECSEL_GIVEN is allowed)", flags)
      return LC_Client_ResultInvalid;
  }
  p2|=0x04;

  dbReq=GWEN_DB_Group_new("ReadRecord");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "recNum", recNum);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "p2", p2);

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



LC_CLIENT_RESULT LC_Card__IsoWriteRecord(LC_CARD *card,
                                         GWEN_TYPE_UINT32 flags,
                                         int recNum,
                                         const char *ptr,
                                         unsigned int size) {
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;

  dbReq=GWEN_DB_Group_new("WriteRecord");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "recNum", recNum);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "p2",
                      ((flags & LC_CARD_ISO_FLAGS_RECSEL_MASK)>>5) |
                      ((flags & LC_CARD_ISO_FLAGS_EFID_MASK)<<3));
  if (ptr && size) {
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                        "data", ptr, size);
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



LC_CLIENT_RESULT LC_Card__IsoUpdateRecord(LC_CARD *card,
                                          GWEN_TYPE_UINT32 flags,
                                          int recNum,
                                          const char *ptr,
                                          unsigned int size) {
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;

  dbReq=GWEN_DB_Group_new("UpdateRecord");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "recNum", recNum);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "p2",
                      ((flags & LC_CARD_ISO_FLAGS_RECSEL_MASK)>>5) |
                      ((flags & LC_CARD_ISO_FLAGS_EFID_MASK)<<3));
  if (ptr && size) {
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                        "data", ptr, size);
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



LC_CLIENT_RESULT LC_Card__IsoAppendRecord(LC_CARD *card,
                                          GWEN_TYPE_UINT32 flags,
                                          const char *ptr,
                                          unsigned int size) {
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;

  dbReq=GWEN_DB_Group_new("UpdateRecord");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "p2",
                      (flags & LC_CARD_ISO_FLAGS_EFID_MASK)<<3);

  if (ptr && size) {
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                        "data", ptr, size);
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



LC_CLIENT_RESULT LC_Card__IsoVerifyPin(LC_CARD *card,
                                       GWEN_TYPE_UINT32 flags,
                                       const LC_PININFO *pi,
                                       const unsigned char *ptr,
                                       unsigned int size,
                                       int *triesLeft) {
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;

  if (triesLeft)
    *triesLeft=-1;

  dbReq=GWEN_DB_Group_new("IsoVerifyPin");
  dbResp=GWEN_DB_Group_new("response");
  LC_PinInfo_toDb(pi, dbReq);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "pid", LC_PinInfo_GetId(pi));

  if (ptr && size) {
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                        "pin", ptr, size);
  }
  res=LC_Card_ExecCommand(card, dbReq, dbResp,
                          LC_Client_GetShortTimeout(LC_Card_GetClient(card)));
  if (res!=LC_Client_ResultOk) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    if (res==LC_Client_ResultCmdError && triesLeft) {
      if (LC_Card_GetLastSW1(card)==0x63) {
        int c;

        c=LC_Card_GetLastSW2(card);
        if (c>=0xc0)
          *triesLeft=(c & 0xf);
      }
    }
    return res;
  }

  GWEN_DB_Group_free(dbResp);
  GWEN_DB_Group_free(dbReq);
  return res;
}



LC_CLIENT_RESULT LC_Card__IsoModifyPin(LC_CARD *card,
                                       GWEN_TYPE_UINT32 flags,
                                       const LC_PININFO *pi,
                                       const unsigned char *oldptr,
                                       unsigned int oldsize,
                                       const unsigned char *newptr,
                                       unsigned int newsize,
                                       int *triesLeft) {
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;

  if (triesLeft)
    *triesLeft=-1;

  dbReq=GWEN_DB_Group_new("IsoModifyPin");
  dbResp=GWEN_DB_Group_new("response");
  LC_PinInfo_toDb(pi, dbReq);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "pid", LC_PinInfo_GetId(pi));

  if (oldptr && oldsize) {
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                        "oldpin", oldptr, oldsize);
  }
  if (newptr && newsize) {
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                        "newpin", newptr, newsize);
  }
  res=LC_Card_ExecCommand(card, dbReq, dbResp,
                          LC_Client_GetShortTimeout(LC_Card_GetClient(card)));
  if (res!=LC_Client_ResultOk) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    if (res==LC_Client_ResultCmdError && triesLeft) {
      if (LC_Card_GetLastSW1(card)==0x63) {
        int c;

        c=LC_Card_GetLastSW2(card);
        if (c>=0xc0)
          *triesLeft=(c & 0xf);
      }
    }
    return res;
  }

  GWEN_DB_Group_free(dbResp);
  GWEN_DB_Group_free(dbReq);
  return res;
}



LC_CLIENT_RESULT LC_Card__IsoPerformVerification(LC_CARD *card,
                                                 GWEN_TYPE_UINT32 flags,
                                                 const LC_PININFO *pi,
                                                 int *triesLeft) {
  GWEN_DB_NODE *dbReq=0;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;

  if (triesLeft)
    *triesLeft=-1;

  switch(LC_PinInfo_GetEncoding(pi)) {
  case LC_PinInfo_EncodingBin:
    dbReq=GWEN_DB_Group_new("IsoPerformVerification_Bin");
    break;
  case LC_PinInfo_EncodingBcd:
    dbReq=GWEN_DB_Group_new("IsoPerformVerification_Bcd");
    break;
  case LC_PinInfo_EncodingAscii:
    dbReq=GWEN_DB_Group_new("IsoPerformVerification_Ascii");
    break;
  case LC_PinInfo_EncodingFpin2:
    dbReq=GWEN_DB_Group_new("IsoPerformVerification_Fpin2");
    break;
  default:
    DBG_ERROR(LC_LOGDOMAIN, "Unhandled pin encoding \"%s\"",
              LC_PinInfo_Encoding_toString(LC_PinInfo_GetEncoding(pi)));
    return LC_Client_ResultInvalid;
  }

  dbResp=GWEN_DB_Group_new("response");
  LC_PinInfo_toDb(pi, dbReq);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "pid", LC_PinInfo_GetId(pi));

  res=LC_Card_ExecCommand(card, dbReq, dbResp,
                          LC_Client_GetShortTimeout(LC_Card_GetClient(card)));
  if (res!=LC_Client_ResultOk) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    if (res==LC_Client_ResultCmdError && triesLeft) {
      if (LC_Card_GetLastSW1(card)==0x63) {
        int c;

        c=LC_Card_GetLastSW2(card);
        if (c>=0xc0)
          *triesLeft=(c & 0xf);
      }
    }
    return res;
  }

  GWEN_DB_Group_free(dbResp);
  GWEN_DB_Group_free(dbReq);
  return res;
}



LC_CLIENT_RESULT LC_Card__IsoPerformModification(LC_CARD *card,
                                                 GWEN_TYPE_UINT32 flags,
                                                 const LC_PININFO *pi,
                                                 int *triesLeft) {
  GWEN_DB_NODE *dbReq=0;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;

  if (triesLeft)
    *triesLeft=-1;

  switch(LC_PinInfo_GetEncoding(pi)) {
  case LC_PinInfo_EncodingBin:
    dbReq=GWEN_DB_Group_new("IsoPerformModification_Bin");
    break;
  case LC_PinInfo_EncodingBcd:
    dbReq=GWEN_DB_Group_new("IsoPerformModification_Bcd");
    break;
  case LC_PinInfo_EncodingAscii:
    dbReq=GWEN_DB_Group_new("IsoPerformModification_Ascii");
    break;
  case LC_PinInfo_EncodingFpin2:
    dbReq=GWEN_DB_Group_new("IsoPerformModification_Fpin2");
    break;
  default:
    DBG_ERROR(LC_LOGDOMAIN, "Unhandled pin encoding \"%s\"",
	      LC_PinInfo_Encoding_toString(LC_PinInfo_GetEncoding(pi)));
    return LC_Client_ResultInvalid;
  }

  dbResp=GWEN_DB_Group_new("response");
  LC_PinInfo_toDb(pi, dbReq);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "pid", LC_PinInfo_GetId(pi));

  res=LC_Card_ExecCommand(card, dbReq, dbResp,
                          LC_Client_GetShortTimeout(LC_Card_GetClient(card)));
  if (res!=LC_Client_ResultOk) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    if (res==LC_Client_ResultCmdError && triesLeft) {
      if (LC_Card_GetLastSW1(card)==0x63) {
        int c;

        c=LC_Card_GetLastSW2(card);
        if (c>=0xc0)
          *triesLeft=(c & 0xf);
      }
    }
    return res;
  }

  GWEN_DB_Group_free(dbResp);
  GWEN_DB_Group_free(dbReq);
  return res;
}



LC_CLIENT_RESULT LC_Card__IsoManageSe(LC_CARD *card,
                                      int tmpl, int kids, int kidp, int ar) {
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;
  GWEN_BUFFER *dbuf;

  assert(card);

  LC_Card_SetLastResult(card, 0, 0, 0, 0);

  dbReq=0;

  dbuf=GWEN_Buffer_new(0, 32, 0, 1);
  if (kids) {
    GWEN_Buffer_AppendByte(dbuf, 0x84);
    GWEN_Buffer_AppendByte(dbuf, 1);
    GWEN_Buffer_AppendByte(dbuf, kids);
  }

  if (kidp) {
    GWEN_Buffer_AppendByte(dbuf, 0x83);
    GWEN_Buffer_AppendByte(dbuf, 1);
    GWEN_Buffer_AppendByte(dbuf, kidp);
  }

  if (ar!=-1) {
    GWEN_Buffer_AppendByte(dbuf, 0x80);
    GWEN_Buffer_AppendByte(dbuf, 1);
    GWEN_Buffer_AppendByte(dbuf, ar);
  }

  dbReq=GWEN_DB_Group_new("ManageSE");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "template", tmpl);
  GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "data",
                      GWEN_Buffer_GetStart(dbuf),
                      GWEN_Buffer_GetUsedBytes(dbuf));
  GWEN_Buffer_free(dbuf);

  dbResp=GWEN_DB_Group_new("response");
  res=LC_Card_ExecCommand(card, dbReq, dbResp,
                          LC_Client_GetShortTimeout(LC_Card_GetClient(card)));
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    return res;
  }
  GWEN_DB_Group_free(dbReq);
  GWEN_DB_Group_free(dbResp);
  return LC_Client_ResultOk;
}




LC_CLIENT_RESULT LC_Card__IsoEncipher(LC_CARD *card,
                                      const char *ptr,
                                      unsigned int size,
                                      GWEN_BUFFER *codeBuf) {
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbRsp;
  LC_CLIENT_RESULT res;
  const void *p;
  unsigned int bs;

  assert(card);

  /* put data */
  dbReq=GWEN_DB_Group_new("Encipher");
  dbRsp=GWEN_DB_Group_new("response");
  GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "data", ptr, size);
  LC_Card_SetLastResult(card, 0, 0, 0, 0);
  res=LC_Card_ExecCommand(card, dbReq, dbRsp,
                          LC_Client_GetShortTimeout(LC_Card_GetClient(card)));
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbRsp);
    return res;
  }

  /* extract the encoded data */
  p=GWEN_DB_GetBinValue(dbRsp, "command/response/data", 0, 0, 0, &bs);
  if (!p || !bs) {
    DBG_ERROR(LC_LOGDOMAIN, "No data returned by card");
    GWEN_DB_Dump(dbRsp, stderr, 2);
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbRsp);
    return res;
  }
  GWEN_Buffer_AppendBytes(codeBuf, p, bs);
  GWEN_DB_Group_free(dbReq);
  GWEN_DB_Group_free(dbRsp);

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_Card__IsoDecipher(LC_CARD *card,
                                      const char *ptr,
                                      unsigned int size,
                                      GWEN_BUFFER *plainBuf) {
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbRsp;
  LC_CLIENT_RESULT res;
  const void *p;
  unsigned int bs;

  assert(card);

  /* put hash */
  dbReq=GWEN_DB_Group_new("Decipher");
  dbRsp=GWEN_DB_Group_new("response");
  GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "data", ptr, size);
  LC_Card_SetLastResult(card, 0, 0, 0, 0);
  res=LC_Card_ExecCommand(card, dbReq, dbRsp,
                          LC_Client_GetShortTimeout(LC_Card_GetClient(card)));
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbRsp);
    return res;
  }

  /* extract the decoded data */
  p=GWEN_DB_GetBinValue(dbRsp, "command/response/data", 0, 0, 0, &bs);
  if (!p || !bs) {
    DBG_ERROR(LC_LOGDOMAIN, "No data returned by card");
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbRsp);
    return res;
  }
  GWEN_Buffer_AppendBytes(plainBuf, p, bs);
  GWEN_DB_Group_free(dbReq);
  GWEN_DB_Group_free(dbRsp);

  return LC_Client_ResultOk;
}






LC_PININFO *LC_Card_GetPinInfoById(LC_CARD *card, GWEN_TYPE_UINT32 pid) {
  GWEN_XMLNODE *n;

  n=LC_Card__GetEfInfo(card);
  if (!n)
    n=LC_Card__GetDfInfo(card);
  if (!n)
    n=LC_Card__GetAppInfo(card);
  if (!n) {
    DBG_INFO(LC_LOGDOMAIN, "No XML info");
    return 0;
  }

  while(n) {
    GWEN_XMLNODE *nn;

    nn=GWEN_XMLNode_FindFirstTag(n, "pins", 0, 0);
    while (nn) {
      GWEN_XMLNODE *nnn;

      nnn=GWEN_XMLNode_FindFirstTag(nn, "pin", 0, 0);
      while(nnn) {
        const char *s;

        s=GWEN_XMLNode_GetProperty(nnn, "id", 0);
        if (s) {
          int i;

          if (sscanf(s, "%i", &i)==1) {
            if (i==(int)pid) {
              LC_PININFO *pi;

              pi=LC_PinInfo_new();
              LC_PinInfo_SetId(pi, pid);
              s=GWEN_XMLNode_GetProperty(nnn, "name", 0);
              LC_PinInfo_SetName(pi, s);
              if (1==sscanf(GWEN_XMLNode_GetProperty(nnn, "minLen", "0"),
                            "%i", &i))
                LC_PinInfo_SetMinLength(pi, i);
              if (1==sscanf(GWEN_XMLNode_GetProperty(nnn, "maxLen", "0"),
                            "%i", &i))
                LC_PinInfo_SetMaxLength(pi, i);
              if (1==sscanf(GWEN_XMLNode_GetProperty(nnn, "allowChange", "0"),
                            "%i", &i))
                LC_PinInfo_SetAllowChange(pi, i);
              if (1==sscanf(GWEN_XMLNode_GetProperty(nnn, "filler", "0"),
                            "%i", &i))
                LC_PinInfo_SetFiller(pi, i);
              s=GWEN_XMLNode_GetProperty(nnn, "encoding", 0);
              if (s)
                LC_PinInfo_SetEncoding(pi, LC_PinInfo_Encoding_fromString(s));
              return pi;
            }
          }
        }
        nnn=GWEN_XMLNode_FindNextTag(nnn, "pin", 0, 0);
      }

      nn=GWEN_XMLNode_FindNextTag(nn, "pins", 0, 0);
    }

    n=GWEN_XMLNode_GetParent(n);
  }

  return 0;
}



LC_PININFO *LC_Card_GetPinInfoByName(LC_CARD *card, const char *name) {
  GWEN_XMLNODE *n;

  n=LC_Card__GetEfInfo(card);
  if (!n)
    n=LC_Card__GetDfInfo(card);
  if (!n)
    n=LC_Card__GetAppInfo(card);
  if (!n) {
    DBG_INFO(LC_LOGDOMAIN, "No XML info");
    return 0;
  }

  while(n) {
    GWEN_XMLNODE *nn;

    nn=GWEN_XMLNode_FindFirstTag(n, "pins", 0, 0);
    while (nn) {
      GWEN_XMLNODE *nnn;

      nnn=GWEN_XMLNode_FindFirstTag(nn, "pin", 0, 0);
      while(nnn) {
        const char *s;
        int i;

        s=GWEN_XMLNode_GetProperty(nnn, "id", 0);
        if (s && sscanf(s, "%i", &i)==1) {
          s=GWEN_XMLNode_GetProperty(nnn, "name", 0);
          if (s && strcasecmp(s, name)==0) {
            LC_PININFO *pi;

            pi=LC_PinInfo_new();
            LC_PinInfo_SetId(pi, (GWEN_TYPE_UINT32)i);
            s=GWEN_XMLNode_GetProperty(nnn, "name", 0);
            LC_PinInfo_SetName(pi, s);
            if (1==sscanf(GWEN_XMLNode_GetProperty(nnn, "minLen", "0"),
                          "%i", &i))
              LC_PinInfo_SetMinLength(pi, i);
            if (1==sscanf(GWEN_XMLNode_GetProperty(nnn, "maxLen", "0"),
                          "%i", &i))
              LC_PinInfo_SetMaxLength(pi, i);
            if (1==sscanf(GWEN_XMLNode_GetProperty(nnn, "allowChange", "0"),
                          "%i", &i))
              LC_PinInfo_SetAllowChange(pi, i);
            if (1==sscanf(GWEN_XMLNode_GetProperty(nnn, "filler", "0"),
                          "%i", &i))
              LC_PinInfo_SetFiller(pi, i);
            s=GWEN_XMLNode_GetProperty(nnn, "encoding", 0);
            if (s)
              LC_PinInfo_SetEncoding(pi, LC_PinInfo_Encoding_fromString(s));
            return pi;
          }
        }
        nnn=GWEN_XMLNode_FindNextTag(nnn, "pin", 0, 0);
      }

      nn=GWEN_XMLNode_FindNextTag(nn, "pins", 0, 0);
    }

    n=GWEN_XMLNode_GetParent(n);
  }

  return 0;
}



LC_CLIENT_RESULT LC_Card_GetPinStatus(LC_CARD *card,
                                      unsigned int pid,
                                      int *maxErrors,
                                      int *currentErrors) {
  assert(card);
  if (card->getPinStatusFn) {
    return card->getPinStatusFn(card, pid, maxErrors, currentErrors);
  }
  else {
    DBG_ERROR(LC_LOGDOMAIN,
              "no getInitialPin function set");
    return LC_Client_ResultNotSupported;
  }
}



LC_CLIENT_RESULT LC_Card_GetInitialPin(LC_CARD *card,
                                       int id,
                                       unsigned char *buffer,
                                       unsigned int maxLen,
                                       unsigned int *pinLength) {
  assert(card);
  if (card->getInitialPinFn) {
    return card->getInitialPinFn(card, id, buffer, maxLen, pinLength);
  }
  else {
    DBG_ERROR(LC_LOGDOMAIN,
              "no getInitialPin function set");
    return LC_Client_ResultNotSupported;
  }
}










