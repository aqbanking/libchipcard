/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: egkcard.c 346 2007-01-24 15:22:22Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "egkcard_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/buffer.h>
#include <gwenhywfar/text.h>
#include <chipcard/chipcard.h>
#include <chipcard/client/cards/processorcard.h>


GWEN_INHERIT(LC_CARD, LC_EGKCARD)



int LC_EgkCard_ExtendCard(LC_CARD *card){
  LC_EGKCARD *egk;
  int rv;

  rv=LC_ProcessorCard_ExtendCard(card);
  if (rv) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not extend card as processor card");
    return rv;
  }

  GWEN_NEW_OBJECT(LC_EGKCARD, egk);

  egk->openFn=LC_Card_GetOpenFn(card);
  egk->closeFn=LC_Card_GetCloseFn(card);
  LC_Card_SetOpenFn(card, LC_EgkCard_Open);
  LC_Card_SetCloseFn(card, LC_EgkCard_Close);

  GWEN_INHERIT_SETDATA(LC_CARD, LC_EGKCARD, card, egk,
                       LC_EgkCard_freeData);
  return 0;
}



int LC_EgkCard_UnextendCard(LC_CARD *card){
  LC_EGKCARD *egk;
  int rv;

  egk=GWEN_INHERIT_GETDATA(LC_CARD, LC_EGKCARD, card);
  assert(egk);
  LC_Card_SetOpenFn(card, egk->openFn);
  LC_Card_SetCloseFn(card, egk->closeFn);
  GWEN_INHERIT_UNLINK(LC_CARD, LC_EGKCARD, card);

  rv=LC_ProcessorCard_UnextendCard(card);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here");
  }
  return rv;
}



void GWENHYWFAR_CB LC_EgkCard_freeData(void *bp, void *p){
  LC_EGKCARD *egk;

  assert(bp);
  assert(p);
  egk=(LC_EGKCARD*)p;
  GWEN_FREE_OBJECT(egk);
}



LC_CLIENT_RESULT CHIPCARD_CB LC_EgkCard_Open(LC_CARD *card){
  LC_CLIENT_RESULT res;
  LC_EGKCARD *egk;

  DBG_INFO(LC_LOGDOMAIN, "Opening card as DDV card");

  assert(card);
  egk=GWEN_INHERIT_GETDATA(LC_CARD, LC_EGKCARD, card);
  assert(egk);

  if (strcasecmp(LC_Card_GetCardType(card), "PROCESSOR")!=0) {
    DBG_ERROR(LC_LOGDOMAIN, "Not a processor card (%s)",
              LC_Card_GetCardType(card));
    return LC_Client_ResultNotSupported;
  }

  res=egk->openFn(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  res=LC_EgkCard_Reopen(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    egk->closeFn(card);
    return res;
  }

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_EgkCard_Reopen(LC_CARD *card){
  LC_CLIENT_RESULT res;
  LC_EGKCARD *egk;

  DBG_INFO(LC_LOGDOMAIN, "Opening eGK card");

  assert(card);
  egk=GWEN_INHERIT_GETDATA(LC_CARD, LC_EGKCARD, card);
  assert(egk);

  res=LC_Card_SelectApp(card, "egk");
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  res=LC_Card_SelectCard(card, 0);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  DBG_INFO(LC_LOGDOMAIN, "Selecting MF...");
  res=LC_Card_SelectMf(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  DBG_INFO(LC_LOGDOMAIN, "Selecting DF...");
  res=LC_Card_SelectDf(card, "DF_HCA");
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_EgkCard_Close(LC_CARD *card){
  LC_CLIENT_RESULT res;
  LC_EGKCARD *egk;

  assert(card);
  egk=GWEN_INHERIT_GETDATA(LC_CARD, LC_EGKCARD, card);
  assert(egk);

  res=egk->closeFn(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  return res;
}



LC_CLIENT_RESULT LC_EgkCard_VerifyPin(LC_CARD *card, const char *pin){
  LC_EGKCARD *egk;
  LC_CLIENT_RESULT res;
  LC_PININFO *pi;
  int triesLeft=-1;

  assert(card);
  egk=GWEN_INHERIT_GETDATA(LC_CARD, LC_EGKCARD, card);
  assert(egk);

  pi=LC_Card_GetPinInfoByName(card, "ch_pin");
  assert(pi);
  res=LC_Card_IsoVerifyPin(card, 0, pi,
			   (const unsigned char*)pin, strlen(pin),
			   &triesLeft);
  LC_PinInfo_free(pi);
  return res;
}



LC_CLIENT_RESULT LC_EgkCard_SecureVerifyPin(LC_CARD *card){
  LC_EGKCARD *egk;
  LC_CLIENT_RESULT res;
  LC_PININFO *pi;
  int triesLeft=-1;

  assert(card);
  egk=GWEN_INHERIT_GETDATA(LC_CARD, LC_EGKCARD, card);
  assert(egk);

  pi=LC_Card_GetPinInfoByName(card, "ch_pin");
  assert(pi);
  res=LC_Card_IsoPerformVerification(card, 0, pi, &triesLeft);
  LC_PinInfo_free(pi);
  return res;
}



LC_CLIENT_RESULT LC_EgkCard_ReadPd(LC_CARD *card, GWEN_BUFFER *buf){
  LC_EGKCARD *egk;
  LC_CLIENT_RESULT res;
  GWEN_BUFFER *lbuf;
  int size;
  const unsigned char *p;

  assert(card);
  egk=GWEN_INHERIT_GETDATA(LC_CARD, LC_EGKCARD, card);
  assert(egk);

  res=LC_Card_SelectEf(card, "EF_PD");
  if (res!=LC_Client_ResultOk)
    return res;

  lbuf=GWEN_Buffer_new(0, 2, 0, 1);
  res=LC_Card_IsoReadBinary(card, 0, 0, 2, lbuf);
  if (res!=LC_Client_ResultOk) {
    GWEN_Buffer_free(lbuf);
    return res;
  }

  if (GWEN_Buffer_GetUsedBytes(lbuf)<2) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid response size (%d)",
	      GWEN_Buffer_GetUsedBytes(lbuf));
    GWEN_Buffer_free(lbuf);
    return LC_Client_ResultDataError;
  }

  p=(const unsigned char*)GWEN_Buffer_GetStart(lbuf);
  assert(p);
  size=(*(p++))<<8;
  size+=*p;
  if (size<2) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid size spec in data (%d)", size);
    GWEN_Buffer_free(lbuf);
    return LC_Client_ResultDataError;
  }
  size-=2;
  GWEN_Buffer_free(lbuf);

  if (size) {
    res=LC_Card_ReadBinary(card, 2, size, buf);
    if (res!=LC_Client_ResultOk) {
      DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
      return res;
    }
  }

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_EgkCard_ReadVd(LC_CARD *card, GWEN_BUFFER *buf){
  LC_EGKCARD *egk;
  LC_CLIENT_RESULT res;
  GWEN_BUFFER *lbuf;
  int offs1, offs2;
  int end1, end2;
  int size1, size2;
  const unsigned char *p;

  assert(card);
  egk=GWEN_INHERIT_GETDATA(LC_CARD, LC_EGKCARD, card);
  assert(egk);

  res=LC_Card_SelectEf(card, "EF_VD");
  if (res!=LC_Client_ResultOk)
    return res;

  lbuf=GWEN_Buffer_new(0, 8, 0, 1);
  res=LC_Card_IsoReadBinary(card, 0, 0, 8, lbuf);
  if (res!=LC_Client_ResultOk) {
    GWEN_Buffer_free(lbuf);
    return res;
  }

  if (GWEN_Buffer_GetUsedBytes(lbuf)<8) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid response size (%d)",
	      GWEN_Buffer_GetUsedBytes(lbuf));
    GWEN_Buffer_free(lbuf);
    return LC_Client_ResultDataError;
  }

  p=(const unsigned char*)GWEN_Buffer_GetStart(lbuf);
  assert(p);
  offs1=(*(p++))<<8;
  offs1+=(*(p++));
  end1=(*(p++))<<8;
  end1+=(*(p++));
  size1=end1-offs1+1;

  offs2=(*(p++))<<8;
  offs2+=(*(p++));
  end2=(*(p++))<<8;
  end2+=(*(p++));
  size2=end2-offs2+1;

  GWEN_Buffer_free(lbuf);

  if (offs1!=0xffff && end1!=0xffff && size1>0) {
    res=LC_Card_ReadBinary(card, offs1, size1, buf);
    if (res!=LC_Client_ResultOk) {
      DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
      return res;
    }
  }

  if (offs2!=0xffff && end2!=0xffff && size2>0) {
    res=LC_Card_ReadBinary(card, offs2, size2, buf);
    if (res!=LC_Client_ResultOk) {
      DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
      return res;
    }
  }

  return LC_Client_ResultOk;
}









