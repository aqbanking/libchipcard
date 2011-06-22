/***************************************************************************
    begin       : Sat Nov 13 2010
    copyright   : (C) 2010 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "zkacard_p.h"

#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/buffer.h>
#include <gwenhywfar/text.h>
#include <chipcard/chipcard.h>
#include <chipcard/cards/processorcard.h>


GWEN_INHERIT(LC_CARD, LC_ZKACARD)



int LC_ZkaCard_ExtendCard(LC_CARD *card) {
  LC_ZKACARD *xc;
  int rv;

  rv=LC_ProcessorCard_ExtendCard(card);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  GWEN_NEW_OBJECT(LC_ZKACARD, xc);
  GWEN_INHERIT_SETDATA(LC_CARD, LC_ZKACARD, card, xc,
		       LC_ZkaCard_freeData);

  LC_Card_SetLastResult(card, 0, 0, 0, 0);
  xc->openFn=LC_Card_GetOpenFn(card);
  xc->closeFn=LC_Card_GetCloseFn(card);
  LC_Card_SetOpenFn(card, LC_ZkaCard_Open);
  LC_Card_SetCloseFn(card, LC_ZkaCard_Close);

  LC_Card_SetGetPinStatusFn(card, LC_ZkaCard_GetPinStatus);

  return 0;
}



int LC_ZkaCard_UnextendCard(LC_CARD *card) {
  LC_ZKACARD *xc;
  int rv;

  xc=GWEN_INHERIT_GETDATA(LC_CARD, LC_ZKACARD, card);
  assert(xc);
  LC_Card_SetOpenFn(card, xc->openFn);
  LC_Card_SetCloseFn(card, xc->closeFn);
  GWEN_INHERIT_UNLINK(LC_CARD, LC_ZKACARD, card);

  rv=LC_ProcessorCard_UnextendCard(card);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here");
  }
  return rv;
}



void GWENHYWFAR_CB LC_ZkaCard_freeData(void *bp, void *p){
  LC_ZKACARD *xc;

  assert(bp);
  assert(p);
  xc=(LC_ZKACARD*)p;

  LC_PinInfo_free(xc->pinInfo);

  GWEN_Buffer_free(xc->bin_ef_id);
  GWEN_DB_Group_free(xc->db_ef_id);

  GWEN_Buffer_free(xc->bin_ef_gd_0);
  GWEN_Buffer_free(xc->bin_ef_ssd);

  GWEN_FREE_OBJECT(xc);
}


int LC_ZkaCard__ParsePseudoOids(const uint8_t *p, uint32_t bs, GWEN_BUFFER *mbuf) {
  GWEN_BUFFER *xbuf;

  xbuf=GWEN_Buffer_new(0, 256, 0, 1);
  while(p && bs) {
    uint8_t x;

    x=*p;
    GWEN_Buffer_AppendByte(xbuf, (x>>4) & 0xf);
    GWEN_Buffer_AppendByte(xbuf, x & 0xf);

    p++;
    bs--;
  }

  p=(const uint8_t*)GWEN_Buffer_GetStart(xbuf);
  bs=GWEN_Buffer_GetUsedBytes(xbuf);
  while(p && bs) {
    uint32_t v=0;
    uint8_t x;

    x=*p;
    v<<=3;
    v|=x & 7;
    if ((x & 8)==0) {
      GWEN_Buffer_AppendByte(mbuf, (v>>24) & 0xff);
      GWEN_Buffer_AppendByte(mbuf, (v>>16) & 0xff);
      GWEN_Buffer_AppendByte(mbuf, (v>>8) & 0xff);
      GWEN_Buffer_AppendByte(mbuf, v & 0xff);
    }
    p++;
    bs--;
  }

  GWEN_Buffer_free(xbuf);
  return 0;
}



LC_CLIENT_RESULT LC_ZkaCard_Reopen(LC_CARD *card) {
  LC_CLIENT_RESULT res;
  LC_ZKACARD *xc;
  GWEN_BUFFER *mbuf;
  GWEN_DB_NODE *dbRecord;

  DBG_INFO(LC_LOGDOMAIN, "Opening ZkaCard card");

  assert(card);
  xc=GWEN_INHERIT_GETDATA(LC_CARD, LC_ZKACARD, card);
  assert(xc);

  LC_Card_SetLastResult(card, 0, 0, 0, 0);
  GWEN_Buffer_free(xc->bin_ef_gd_0);
  xc->bin_ef_gd_0=NULL;

  GWEN_Buffer_free(xc->bin_ef_id);
  xc->bin_ef_id=NULL;

  GWEN_DB_Group_free(xc->db_ef_id);
  xc->db_ef_id=NULL;

  GWEN_Buffer_free(xc->bin_ef_ssd);
  xc->bin_ef_ssd=NULL;

  LC_PinInfo_free(xc->pinInfo);
  xc->pinInfo=NULL;
  xc->pinRecordNum=-1;

  /* select ZKA card */
  res=LC_Card_SelectCard(card, "zkacard");
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  /* select ZKA app */
  res=LC_Card_SelectApp(card, "zkacard");
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  /* select MF */
  DBG_INFO(LC_LOGDOMAIN, "Selecting MF...");
  res=LC_Card_SelectMf(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  /* read EF_ID */
  DBG_INFO(LC_LOGDOMAIN, "Selecting EF_ID...");
  res=LC_Card_SelectEf(card, "EF_ID");
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  /* parse EF_ID */
  DBG_INFO(LC_LOGDOMAIN, "Reading record...");
  mbuf=GWEN_Buffer_new(0, 32, 0, 1);
  res=LC_Card_IsoReadRecord(card, LC_CARD_ISO_FLAGS_RECSEL_GIVEN, 1, mbuf);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_Buffer_free(mbuf);
    return res;
  }
  xc->bin_ef_id=mbuf;

  DBG_INFO(LC_LOGDOMAIN, "Parsing record...");
  GWEN_Buffer_Rewind(mbuf);
  dbRecord=GWEN_DB_Group_new("record");
  if (LC_Card_ParseRecord(card, 1, mbuf, dbRecord)) {
    DBG_ERROR(LC_LOGDOMAIN, "Error in EF_ID");
    GWEN_DB_Group_free(dbRecord);
    GWEN_Buffer_free(mbuf);
    return LC_Client_ResultDataError;
  }

  xc->db_ef_id=dbRecord;


  /* read EG_GD0 */
  DBG_INFO(LC_LOGDOMAIN, "Selecting EF_GD0...");
  res=LC_Card_SelectEf(card, "EF_GD0");
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  DBG_INFO(LC_LOGDOMAIN, "Reading data...");
  mbuf=GWEN_Buffer_new(0, 16, 0, 1);
  res=LC_Card_IsoReadBinary(card, 0, 0, 12, mbuf);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_Buffer_free(mbuf);
    return res;
  }
  if (GWEN_Buffer_GetUsedBytes(mbuf)<12) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_Buffer_free(mbuf);
    return LC_Client_ResultDataError;
  }
  xc->bin_ef_gd_0=mbuf;

  /* read EF_PWDD */
  DBG_INFO(LC_LOGDOMAIN, "Selecting EF_PWDD...");
  res=LC_Card_SelectEf(card, "EF_PWDD");
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  /* read EF_PWDD */
  DBG_INFO(LC_LOGDOMAIN, "Reading record...");
  mbuf=GWEN_Buffer_new(0, 32, 0, 1);
  res=LC_Card_IsoReadRecord(card, LC_CARD_ISO_FLAGS_RECSEL_GIVEN, 1, mbuf);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_Buffer_free(mbuf);
    return res;
  }

  /* parse EF_PWDD */
  DBG_INFO(LC_LOGDOMAIN, "Parsing record...");
  GWEN_Buffer_Rewind(mbuf);
  dbRecord=GWEN_DB_Group_new("record");
  if (LC_Card_ParseRecord(card, 1, mbuf, dbRecord)) {
    DBG_ERROR(LC_LOGDOMAIN, "Error in EF_PWDD");
    GWEN_DB_Group_free(dbRecord);
    GWEN_Buffer_free(mbuf);
    return LC_Client_ResultDataError;
  }
  else {
    const uint8_t *p;
    uint32_t bs;
    LC_PININFO *pi;
    int i;

    GWEN_Buffer_free(mbuf);
    i=GWEN_DB_GetIntValue(dbRecord, "entry/pwdRecord", 0, -1);
    if (i>0)
      xc->pinRecordNum=i;
    pi=LC_PinInfo_new();
    LC_PinInfo_SetAllowChange(pi, 1);
    i=GWEN_DB_GetIntValue(dbRecord, "entry/pwdId", 0, -1);
    if (i>0)
      LC_PinInfo_SetId(pi, i);
    p=GWEN_DB_GetBinValue(dbRecord, "entry/format", 0, NULL, 0, &bs);
    if (p && bs) {
      GWEN_BUFFER *obuf;
      int rv;
      uint32_t v;

      obuf=GWEN_Buffer_new(0, 64, 0, 1);
      rv=LC_ZkaCard__ParsePseudoOids(p, bs, obuf);
      if (rv<0) {
        GWEN_Buffer_free(obuf);
        LC_PinInfo_free(pi);
        GWEN_DB_Group_free(dbRecord);
        return rv;
      }
      p=(const uint8_t*) GWEN_Buffer_GetStart(obuf);
      bs=GWEN_Buffer_GetUsedBytes(obuf);
      if (bs>=8) {
        uint32_t v2;

        v=(uint32_t)(p[0])<<24;
        v+=(uint32_t)(p[1])<<16;
        v+=(uint32_t)(p[2])<<8;
        v+=(uint32_t)(p[3]);

        v2=(uint32_t)(p[4])<<24;
        v2+=(uint32_t)(p[5])<<16;
        v2+=(uint32_t)(p[6])<<8;
        v2+=(uint32_t)(p[7]);

        if (v==2 && v2==1)
          LC_PinInfo_SetEncoding(pi, GWEN_Crypt_PinEncoding_Ascii);
        else if (v==1 && v2==1)
          LC_PinInfo_SetEncoding(pi, GWEN_Crypt_PinEncoding_FPin2);
        else {
          DBG_WARN(LC_LOGDOMAIN, "Unexpected encoding info (%d/%d), assuming Ascii",
                   (int) v, (int) v2);
          LC_PinInfo_SetEncoding(pi, GWEN_Crypt_PinEncoding_Ascii);
        }
      }

      if (bs>=12) {
        v=(uint32_t)(p[8])<<24;
        v+=(uint32_t)(p[9])<<16;
        v+=(uint32_t)(p[10])<<8;
        v+=(uint32_t)(p[11]);

        LC_PinInfo_SetMinLength(pi, v);
      }

      GWEN_Buffer_free(obuf);
      xc->pinInfo=pi;

#if 1
      if (1) {
        GWEN_DB_NODE *dbD;

        DBG_ERROR(LC_LOGDOMAIN, "Got this pininfo:");
        dbD=GWEN_DB_Group_new("debug");
        LC_PinInfo_toDb(xc->pinInfo, dbD);
        GWEN_DB_Dump(dbD, 2);
        GWEN_DB_Group_free(dbD);
      }
#endif
    }
    GWEN_DB_Group_free(dbRecord);
  }


  /* select DF_SIG */
  DBG_INFO(LC_LOGDOMAIN, "Selecting DF_SIG...");
  res=LC_Card_SelectDf(card, "DF_SIG");
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  /* read EG_SSD */
  DBG_INFO(LC_LOGDOMAIN, "Selecting EF_SSD...");
  res=LC_Card_SelectEf(card, "EF_SSD");
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  DBG_INFO(LC_LOGDOMAIN, "Reading data...");
  mbuf=GWEN_Buffer_new(0, 16, 0, 1);
  res=LC_Card_ReadBinary(card, 0, 65535, mbuf);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
    GWEN_Buffer_free(mbuf);
    return res;
  }
  xc->bin_ef_ssd=mbuf;



  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ZkaCard_Open(LC_CARD *card){
  LC_CLIENT_RESULT res;
  LC_ZKACARD *xc;

  DBG_INFO(LC_LOGDOMAIN, "Opening card as ZkaCard card");

  assert(card);
  xc=GWEN_INHERIT_GETDATA(LC_CARD, LC_ZKACARD, card);
  assert(xc);

  LC_Card_SetLastResult(card, 0, 0, 0, 0);

  GWEN_Buffer_free(xc->bin_ef_gd_0);
  xc->bin_ef_gd_0=NULL;

  GWEN_Buffer_free(xc->bin_ef_id);
  xc->bin_ef_id=NULL;

  GWEN_Buffer_free(xc->bin_ef_ssd);
  xc->bin_ef_ssd=NULL;

  res=xc->openFn(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  res=LC_ZkaCard_Reopen(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    xc->closeFn(card);
    return res;
  }

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ZkaCard_Close(LC_CARD *card){
  LC_CLIENT_RESULT res;
  LC_ZKACARD *xc;

  assert(card);
  xc=GWEN_INHERIT_GETDATA(LC_CARD, LC_ZKACARD, card);
  assert(xc);

  LC_Card_SetLastResult(card, 0, 0, 0, 0);
  res=xc->closeFn(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  return res;
}



const LC_PININFO *LC_ZkaCard_GetPinInfo(const LC_CARD *card) {
  LC_ZKACARD *xc;

  assert(card);
  xc=GWEN_INHERIT_GETDATA(LC_CARD, LC_ZKACARD, card);
  assert(xc);

  return xc->pinInfo;
}



GWEN_DB_NODE *LC_ZkaCard_GetCardDataAsDb(const LC_CARD *card) {
  LC_ZKACARD *xc;

  assert(card);
  xc=GWEN_INHERIT_GETDATA(LC_CARD, LC_ZKACARD, card);
  assert(xc);

  return xc->db_ef_id;
}



GWEN_BUFFER *LC_ZkaCard_GetCardDataAsBuffer(const LC_CARD *card) {
  LC_ZKACARD *xc;

  assert(card);
  xc=GWEN_INHERIT_GETDATA(LC_CARD, LC_ZKACARD, card);
  assert(xc);

  return xc->bin_ef_id;
}




LC_CLIENT_RESULT LC_ZkaCard__PrepareSign(LC_CARD *card, int globalKey, int keyId, int keyVersion) {
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  LC_CLIENT_RESULT res;
  GWEN_BUFFER *dbuf;

  assert(card);

  LC_Card_SetLastResult(card, 0, 0, 0, 0);

  dbuf=GWEN_Buffer_new(0, 32, 0, 1);
  GWEN_Buffer_AppendByte(dbuf, 0x84);
  GWEN_Buffer_AppendByte(dbuf, 3);
  GWEN_Buffer_AppendByte(dbuf, globalKey?0x00:0x80);
  GWEN_Buffer_AppendByte(dbuf, keyId);
  GWEN_Buffer_AppendByte(dbuf, (keyVersion>=0)?keyVersion:0xff);

  GWEN_Buffer_AppendByte(dbuf, 0x89);
  GWEN_Buffer_AppendByte(dbuf, 3);
  GWEN_Buffer_AppendByte(dbuf, 23);
  GWEN_Buffer_AppendByte(dbuf, 53);
  GWEN_Buffer_AppendByte(dbuf, 30);

  dbReq=GWEN_DB_Group_new("request");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "template", 0x41);
  GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "data",
                      GWEN_Buffer_GetStart(dbuf),
                      GWEN_Buffer_GetUsedBytes(dbuf));
  GWEN_Buffer_free(dbuf);

  dbResp=GWEN_DB_Group_new("response");
  res=LC_Card_ExecCommand(card, "IsoManageSE", dbReq, dbResp);
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



LC_CLIENT_RESULT LC_ZkaCard_Sign(LC_CARD *card,
                                 int globalKey,
                                 int keyId,
                                 int keyVersion,
                                 const uint8_t *ptr,
                                 unsigned int size,
                                 GWEN_BUFFER *sigBuf) {
  LC_CLIENT_RESULT res;
  int combinedKid;

  assert(card);

  combinedKid=keyId;
  if (globalKey)
    combinedKid|=0x80;
  res=LC_ZkaCard__PrepareSign(card, globalKey, keyId, keyVersion);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  res=LC_Card_IsoInternalAuth(card, combinedKid, ptr, size, sigBuf);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT CHIPCARD_CB 
LC_ZkaCard_GetPinStatus(LC_CARD *card,
                        unsigned int pid,
                        int *maxErrors,
                        int *currentErrors) {
  LC_ZKACARD *xc;

  assert(card);
  xc=GWEN_INHERIT_GETDATA(LC_CARD, LC_ZKACARD, card);
  assert(xc);

  if (xc->pinRecordNum>0) {
    LC_CLIENT_RESULT res;
    GWEN_BUFFER *mbuf;
    const uint8_t *p;

    /* select MF */
    DBG_INFO(LC_LOGDOMAIN, "Selecting MF...");
    res=LC_Card_SelectMf(card);
    if (res!=LC_Client_ResultOk) {
      DBG_INFO(LC_LOGDOMAIN, "here");
      return res;
    }

    /* read EF_ID */
    DBG_INFO(LC_LOGDOMAIN, "Selecting EF_FBZ...");
    res=LC_Card_SelectEf(card, "EF_FBZ");
    if (res!=LC_Client_ResultOk) {
      DBG_INFO(LC_LOGDOMAIN, "here");
      return res;
    }

    /* read EF_FBZ */
    DBG_INFO(LC_LOGDOMAIN, "Reading record...");
    mbuf=GWEN_Buffer_new(0, 32, 0, 1);
    res=LC_Card_IsoReadRecord(card, LC_CARD_ISO_FLAGS_RECSEL_GIVEN, xc->pinRecordNum, mbuf);
    if (res!=LC_Client_ResultOk) {
      DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
      GWEN_Buffer_free(mbuf);
      return res;
    }

    if (GWEN_Buffer_GetUsedBytes(mbuf)<2) {
      DBG_ERROR(LC_LOGDOMAIN, "Too few bytes returned (%d)", GWEN_Buffer_GetUsedBytes(mbuf));
      GWEN_Buffer_free(mbuf);
      return LC_Client_ResultDataError;
    }

    p=(const uint8_t*) GWEN_Buffer_GetStart(mbuf);
    if (maxErrors)
      *maxErrors=p[0];
    if (currentErrors)
      *currentErrors=p[1];
    GWEN_Buffer_free(mbuf);
    return LC_Client_ResultOk;
  }
  else {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid record number for PIN (%d)", xc->pinRecordNum);
    return LC_Client_ResultInternal;
  }
}




