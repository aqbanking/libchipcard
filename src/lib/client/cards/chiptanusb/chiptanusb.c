/***************************************************************************
    begin       : Thu Jan 09 2020
    copyright   : (C) 2020 by Herbert Ellebruch
    email       :

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "chiptanusb_p.h"

#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/tlv.h>
#include <gwenhywfar/buffer.h>
#include <gwenhywfar/text.h>
#include <chipcard/chipcard.h>
#include <chipcard/cards/processorcard.h>


GWEN_INHERIT(LC_CARD, LC_CHIPTANCARD)

int LC_ChiptanusbCard_ExtendCard(LC_CARD *card)
{
  LC_CHIPTANCARD *gk;
  int rv;

  rv=LC_ProcessorCard_ExtendCard(card);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  GWEN_NEW_OBJECT(LC_CHIPTANCARD, gk);
  GWEN_INHERIT_SETDATA(LC_CARD, LC_CHIPTANCARD, card, gk,
                       LC_ChiptanusbCard_freeData);

  gk->openFn=LC_Card_GetOpenFn(card);
  gk->closeFn=LC_Card_GetCloseFn(card);
  LC_Card_SetOpenFn(card, LC_ChiptanusbCard_Open);
  LC_Card_SetCloseFn(card, LC_ChiptanusbCard_Close);

  return 0;
}

int LC_ChiptanusbCard_UnextendCard(LC_CARD *card)
{
  LC_CHIPTANCARD *gk;
  int rv;

  gk=GWEN_INHERIT_GETDATA(LC_CARD, LC_CHIPTANCARD, card);
  assert(gk);
  LC_Card_SetOpenFn(card, gk->openFn);
  LC_Card_SetCloseFn(card, gk->closeFn);
  GWEN_INHERIT_UNLINK(LC_CARD, LC_CHIPTANCARD, card);

  rv=LC_ProcessorCard_UnextendCard(card);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here");
  }

  return rv;
}

void GWENHYWFAR_CB LC_ChiptanusbCard_freeData(void *bp, void *p)
{
  LC_CHIPTANCARD *gk;

  assert(bp);
  assert(p);
  gk=(LC_CHIPTANCARD *)p;

  GWEN_FREE_OBJECT(gk);
}

LC_CLIENT_RESULT LC_ChiptanusbCard_Reopen(LC_CARD *card)
{
  LC_CLIENT_RESULT res;
  LC_CHIPTANCARD *gk;
  GWEN_BUFFER *mbuf;
  GWEN_DB_NODE *dbRecord;

  DBG_INFO(LC_LOGDOMAIN, "Re-Opening Ciptanusb card");

  assert(card);
  gk=GWEN_INHERIT_GETDATA(LC_CARD, LC_CHIPTANCARD, card);
  assert(gk);

  /* select Tan card */
  res=LC_Card_SelectCard(card, "ChiptanusbCard");
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  /* select UsbTan app */
  res=LC_Card_SelectApp(card, "chiptanusbcard");
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  return LC_Client_ResultOk;
}

LC_CLIENT_RESULT CHIPCARD_CB LC_ChiptanusbCard_Open(LC_CARD *card)
{
  LC_CLIENT_RESULT res;
  LC_CHIPTANCARD *gk;

  DBG_INFO(LC_LOGDOMAIN, "Opening card as Chiptanusb Card");

  assert(card);
  gk=GWEN_INHERIT_GETDATA(LC_CARD, LC_CHIPTANCARD, card);
  assert(gk);

  if (strcasecmp(LC_Card_GetCardType(card), "PROCESSOR")!=0) {
    DBG_ERROR(LC_LOGDOMAIN, "Not a processor card");
    return LC_Client_ResultNotSupported;
  }

  res=gk->openFn(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  res= LC_ChiptanusbCard_Reopen(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    gk->closeFn(card);
    return res;
  }

  return LC_Client_ResultOk;
}

LC_CLIENT_RESULT CHIPCARD_CB LC_ChiptanusbCard_Close(LC_CARD *card)
{
  LC_CLIENT_RESULT res;
  LC_CHIPTANCARD *gk;

  assert(card);
  gk=GWEN_INHERIT_GETDATA(LC_CARD, LC_CHIPTANCARD, card);
  assert(gk);

  LC_Card_SetLastResult(card, 0, 0, 0, 0);
  res=gk->closeFn(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  return res;
}

LC_CLIENT_RESULT LC_ChiptanusbCard_GenerateTan(LC_CARD *card,
                                               unsigned char *pCommand, int size, GWEN_BUFFER *buf)
{

  static char QuitString[] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  GWEN_DB_NODE *dbReqQuit;
  GWEN_DB_NODE *dbRespQuit;
  LC_CLIENT_RESULT res;
  dbReq=GWEN_DB_Group_new("request");
  dbResp=GWEN_DB_Group_new("response");
  const void *p;
  unsigned int bs;

  if (pCommand && size) {
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                        "data", pCommand, size);
  }
  res = LC_Card_ExecCommand(card, "GenerateTan", dbReq, dbResp);
  if (res!=LC_Client_ResultOk) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    return res;
  }

  dbReqQuit=GWEN_DB_Group_new("request");
  dbRespQuit=GWEN_DB_Group_new("response");
  GWEN_DB_SetBinValue(dbReqQuit, GWEN_DB_FLAGS_DEFAULT,
                      "data", QuitString, sizeof(QuitString));

  res = LC_Card_ExecCommand(card, "QuitTanResponce", dbReqQuit, dbRespQuit);

  if (res!=LC_Client_ResultOk) {
    return res;
  }


  /* successful */
  p=GWEN_DB_GetBinValue(dbResp,
                        "response/data",
                        0,
                        0, 0,
                        &bs);
  if (p && bs) {
    GWEN_Buffer_AppendBytes(buf, p, bs);
  }
  else {
    DBG_WARN(LC_LOGDOMAIN, "No data in response");
  }
  GWEN_DB_Group_free(dbResp);
  GWEN_DB_Group_free(dbReq);
  return res;

  return LC_Client_ResultOk;
}


