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
#include <gwenhywfar/tlv.h>
#include <gwenhywfar/buffer.h>
#include <gwenhywfar/text.h>
#include <chipcard/chipcard.h>
#include <chipcard/cards/processorcard.h>


GWEN_INHERIT(LC_CARD, LC_ZKACARD)



int LC_ZkaCard_ExtendCard(LC_CARD *card)
{
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

  xc->pinInfoList=LC_PinInfo_List_new();

  return 0;
}



int LC_ZkaCard_UnextendCard(LC_CARD *card)
{
  LC_ZKACARD *xc;
  int rv;

  xc=GWEN_INHERIT_GETDATA(LC_CARD, LC_ZKACARD, card);
  assert(xc);
  LC_Card_SetOpenFn(card, xc->openFn);
  LC_Card_SetCloseFn(card, xc->closeFn);
  GWEN_INHERIT_UNLINK(LC_CARD, LC_ZKACARD, card);

  LC_PinInfo_List_free(xc->pinInfoList);

  rv=LC_ProcessorCard_UnextendCard(card);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here");
  }

  return rv;
}



void GWENHYWFAR_CB LC_ZkaCard_freeData(void *bp, void *p)
{
  LC_ZKACARD *xc;

  assert(bp);
  assert(p);
  xc=(LC_ZKACARD *)p;

  GWEN_Buffer_free(xc->bin_ef_id);
  GWEN_DB_Group_free(xc->db_ef_id);

  GWEN_Buffer_free(xc->bin_ef_gd_0);
  GWEN_Buffer_free(xc->bin_ef_ssd);

  GWEN_FREE_OBJECT(xc);
}


int LC_ZkaCard__ParsePseudoOids(const uint8_t *p, uint32_t bs, GWEN_BUFFER *mbuf)
{
  GWEN_BUFFER *xbuf;

  xbuf=GWEN_Buffer_new(0, 256, 0, 1);
  while (p && bs) {
    uint8_t x;

    x=*p;
    GWEN_Buffer_AppendByte(xbuf, (x>>4) & 0xf);
    GWEN_Buffer_AppendByte(xbuf, x & 0xf);

    p++;
    bs--;
  }

  p=(const uint8_t *)GWEN_Buffer_GetStart(xbuf);
  bs=GWEN_Buffer_GetUsedBytes(xbuf);
  while (p && bs) {
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



LC_CLIENT_RESULT LC_ZkaCard_Reopen(LC_CARD *card)
{
  LC_CLIENT_RESULT res;
  LC_ZKACARD *xc;
  GWEN_BUFFER *mbuf;
  GWEN_DB_NODE *dbRecord;

  DBG_INFO(LC_LOGDOMAIN, "Re-Opening ZkaCard card");

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

  LC_PinInfo_List_Clear(xc->pinInfoList);

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
  res=LC_ZkaCard__ReadPwdd(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  /* select DF_SIG */
  DBG_INFO(LC_LOGDOMAIN, "Selecting DF_SIG...");
  res=LC_Card_SelectDf(card, "DF_SIG");
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  /* read EF_SSD */
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

  LC_ZkaCard__ParseDfSigSSD(card);


  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ZkaCard_Open(LC_CARD *card)
{
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

  LC_PinInfo_List_Clear(xc->pinInfoList);

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



LC_CLIENT_RESULT CHIPCARD_CB LC_ZkaCard_Close(LC_CARD *card)
{
  LC_CLIENT_RESULT res;
  LC_ZKACARD *xc;

  assert(card);
  xc=GWEN_INHERIT_GETDATA(LC_CARD, LC_ZKACARD, card);
  assert(xc);

  LC_Card_SetLastResult(card, 0, 0, 0, 0);
  LC_PinInfo_List_Clear(xc->pinInfoList);
  res=xc->closeFn(card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  return res;
}



const LC_PININFO *LC_ZkaCard_GetPinInfo(const LC_CARD *card, int pid)
{
  LC_ZKACARD *xc;
  const LC_PININFO *pi;

  assert(card);
  xc=GWEN_INHERIT_GETDATA(LC_CARD, LC_ZKACARD, card);
  assert(xc);

  pi=LC_PinInfo_List_First(xc->pinInfoList);
  while (pi) {
    if (LC_PinInfo_GetId(pi)==pid)
      return pi;
    pi=LC_PinInfo_List_Next(pi);
  }

  return NULL;
}



GWEN_DB_NODE *LC_ZkaCard_GetCardDataAsDb(const LC_CARD *card)
{
  LC_ZKACARD *xc;

  assert(card);
  xc=GWEN_INHERIT_GETDATA(LC_CARD, LC_ZKACARD, card);
  assert(xc);

  return xc->db_ef_id;
}



GWEN_DB_NODE *LC_ZkaCard_GetDfSigSsdDataAsDb(const LC_CARD *card)
{
  LC_ZKACARD *xc;

  assert(card);
  xc=GWEN_INHERIT_GETDATA(LC_CARD, LC_ZKACARD, card);
  assert(xc);

  return xc->db_ef_ssd;
}



GWEN_BUFFER *LC_ZkaCard_GetCardDataAsBuffer(const LC_CARD *card)
{
  LC_ZKACARD *xc;

  assert(card);
  xc=GWEN_INHERIT_GETDATA(LC_CARD, LC_ZKACARD, card);
  assert(xc);

  return xc->bin_ef_id;
}




LC_CLIENT_RESULT LC_ZkaCard_Sign(LC_CARD *card,
                                 int globalKey,
                                 int keyId,
                                 int keyVersion,
                                 const uint8_t *ptr,
                                 unsigned int size,
                                 GWEN_BUFFER *sigBuf)
{
  LC_CLIENT_RESULT res;
  int combinedKid;
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;


  assert(card);

  uint8_t *jptr;
  jptr= (uint8_t *) ptr;

  combinedKid=keyId;
  if (globalKey)
    combinedKid|=0x80;

  res=LC_Card_IsoManageSe(card, 0xA4, combinedKid, 0, 0x46);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  dbReq=GWEN_DB_Group_new("request");
  dbResp=GWEN_DB_Group_new("response");

  if (ptr && size) {
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                        "data", ptr, size);
  }

  res=LC_Card_ExecCommand(card, "IsoInternalAuth", dbReq, dbResp);

  //GWEN_DB_Dump(dbReq, 2);
  //GWEN_DB_Dump(dbResp, 2);

  if (res!=LC_Client_ResultOk) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    return res;
  }


  if (sigBuf) {
    unsigned int bs;
    const void *p;

    p=GWEN_DB_GetBinValue(dbResp,
                          "response/data",
                          0,
                          0, 0,
                          &bs);
    if (p && bs) {
      GWEN_Buffer_AppendBytes(sigBuf, p, bs);
    }
    else {
      DBG_WARN(LC_LOGDOMAIN, "No data in response");
    }
  }

  GWEN_DB_Group_free(dbResp);
  GWEN_DB_Group_free(dbReq);
  return res;

  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_ZkaCard_Decipher(LC_CARD *card,
                                     int globalKey,
                                     int keyId,
                                     int keyVersion,
                                     const uint8_t *ptr,
                                     unsigned int size,
                                     GWEN_BUFFER *outBuf)
{
  LC_CLIENT_RESULT res;
  int combinedKid;
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;


  assert(card);

  uint8_t *jptr;
  jptr= (uint8_t *) ptr;

  combinedKid=keyId;
  if (globalKey)
    combinedKid|=0x80;

  res=LC_Card_IsoManageSe(card, 0xB8, combinedKid, 0, 0x1a);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  dbReq=GWEN_DB_Group_new("request");
  dbResp=GWEN_DB_Group_new("response");

  if (ptr && size) {
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                        "data", ptr, size);
  }

  res=LC_Card_ExecCommand(card, "IsoDecipher", dbReq, dbResp);

  if (res!=LC_Client_ResultOk) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    return res;
  }


  if (outBuf) {
    unsigned int bs;
    const void *p;

    p=GWEN_DB_GetBinValue(dbResp,
                          "response/data",
                          0,
                          0, 0,
                          &bs);
    if (p && bs) {
      GWEN_Buffer_AppendBytes(outBuf, p, bs);
    }
    else {
      DBG_WARN(LC_LOGDOMAIN, "No data in response");
    }
  }

  GWEN_DB_Group_free(dbResp);
  GWEN_DB_Group_free(dbReq);
  return res;

  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ZkaCard_GetPinStatus(LC_CARD *card,
                                                     unsigned int pid,
                                                     int *maxErrors,
                                                     int *currentErrors)
{
  LC_ZKACARD *xc;
  const LC_PININFO *pi;

  assert(card);
  xc=GWEN_INHERIT_GETDATA(LC_CARD, LC_ZKACARD, card);
  assert(xc);

  pi=LC_ZkaCard_GetPinInfo(card, pid);
  if (pi && LC_PinInfo_GetRecordNum(pi)>0) {
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
    res=LC_Card_IsoReadRecord(card, LC_CARD_ISO_FLAGS_RECSEL_GIVEN, LC_PinInfo_GetRecordNum(pi), mbuf);
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

    p=(const uint8_t *) GWEN_Buffer_GetStart(mbuf);
    if (maxErrors)
      *maxErrors=p[0];
    if (currentErrors)
      *currentErrors=p[1];
    GWEN_Buffer_free(mbuf);
    return LC_Client_ResultOk;
  }
  else {
    DBG_ERROR(LC_LOGDOMAIN, "No pin or invalid record number for PIN %d", pid);
    return LC_Client_ResultInternal;
  }
}



int LC_ZkaCard__ReadPwdd(LC_CARD *card)
{
  LC_ZKACARD *xc;
  LC_CLIENT_RESULT res;
  int rec;

  assert(card);
  xc=GWEN_INHERIT_GETDATA(LC_CARD, LC_ZKACARD, card);
  assert(xc);

  LC_PinInfo_List_Clear(xc->pinInfoList);

  /* select EF_PWDD */
  DBG_INFO(LC_LOGDOMAIN, "Selecting EF_PWDD...");
  res=LC_Card_SelectEf(card, "EF_PWDD");
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return res;
  }

  /* read EF_PWDD */
  for (rec=1; rec<32; rec++) {
    GWEN_BUFFER *mbuf;
    GWEN_DB_NODE *dbRecord;

    DBG_INFO(LC_LOGDOMAIN, "Reading record %d", rec);
    mbuf=GWEN_Buffer_new(0, 32, 0, 1);
    res=LC_Card_IsoReadRecord(card, LC_CARD_ISO_FLAGS_RECSEL_GIVEN, rec, mbuf);
    if (res!=LC_Client_ResultOk) {
      if (LC_Card_GetLastSW1(card)==0x6a &&
          LC_Card_GetLastSW2(card)==0x83) {
        DBG_INFO(LC_LOGDOMAIN, "All records read (%d)", rec-1);
        break;
      }
      DBG_INFO(LC_LOGDOMAIN, "here");
      GWEN_Buffer_free(mbuf);
      return res;
    }

    /* parse EF_PWDD */
    DBG_INFO(LC_LOGDOMAIN, "Parsing record...");
    GWEN_Buffer_Rewind(mbuf);
    dbRecord=GWEN_DB_Group_new("record");
    if (LC_Card_ParseRecord(card, rec, mbuf, dbRecord)) {
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

      DBG_INFO(GWEN_LOGDOMAIN, "PWDD entry %d:", rec);
      //GWEN_DB_Dump(dbRecord, 2);

      GWEN_Buffer_free(mbuf);
      i=GWEN_DB_GetIntValue(dbRecord, "entry/pwdRecord", 0, -1);
      pi=LC_PinInfo_new();
      LC_PinInfo_SetAllowChange(pi, 1);
      LC_PinInfo_SetRecordNum(pi, i);
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
        p=(const uint8_t *) GWEN_Buffer_GetStart(obuf);
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
        LC_PinInfo_List_Add(pi, xc->pinInfoList);
#if 1
        if (1) {
          GWEN_DB_NODE *dbD;

          DBG_INFO(LC_LOGDOMAIN, "Got this pininfo:");
          dbD=GWEN_DB_Group_new("debug");
          LC_PinInfo_toDb(pi, dbD);
          //GWEN_DB_Dump(dbD, 2);
          GWEN_DB_Group_free(dbD);
        }
#endif
      }
      GWEN_DB_Group_free(dbRecord);
    }
  }

  return LC_Client_ResultOk;
}

LC_CLIENT_RESULT LC_ZkaCard__SeccosSearchRecord(LC_CARD *card,
                                                uint32_t flags,
                                                int recNum,
                                                const char *searchPattern,
                                                unsigned int searchPatternSize,
                                                GWEN_BUFFER *buf)
{
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
              "Invalid flags %u"
              " (only RECSEL_GIVEN is allowed)", flags)
    return LC_Client_ResultInvalid;
  }
  p2|=0x04;

  dbReq=GWEN_DB_Group_new("request");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "recNum", recNum);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "p2", p2);
  if (searchPattern && searchPatternSize) {
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                        "data", searchPattern, searchPatternSize);
  }
  res=LC_Card_ExecCommand(card, "SeccosSearchRecord", dbReq, dbResp);
  if (res!=LC_Client_ResultOk) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    return res;
  }

  /* successful */
  if (buf) {
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
  }

  GWEN_DB_Group_free(dbResp);
  GWEN_DB_Group_free(dbReq);
  return res;
}

LC_CLIENT_RESULT LC_ZkaCard__ParseDfSigSSD(LC_CARD *card)
{
  LC_CLIENT_RESULT res= LC_Client_ResultOk;
  LC_ZKACARD *xc;
  GWEN_BUFFER *mbuf;
  GWEN_DB_NODE *dbRecord;
  int remLen;

  DBG_INFO(LC_LOGDOMAIN, "Parsing ");

  assert(card);
  xc=GWEN_INHERIT_GETDATA(LC_CARD, LC_ZKACARD, card);
  assert(xc);
  GWEN_Buffer_Rewind(xc->bin_ef_ssd);
  dbRecord=GWEN_DB_Group_new("SSD");
  remLen=GWEN_TLV_Buffer_To_DB(dbRecord, xc->bin_ef_ssd, GWEN_Buffer_GetUsedBytes(xc->bin_ef_ssd));

  if (remLen!=GWEN_Buffer_GetUsedBytes(xc->bin_ef_ssd)) {
    DBG_WARN(LC_LOGDOMAIN, "tlv buffer not completely parsed!");
  }

  xc->db_ef_ssd=dbRecord;
  /*GWEN_DB_Dump(dbRecord,4);
   GWEN_DB_Group_free(dbRecord);*/
  return res;
}


