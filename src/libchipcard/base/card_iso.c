/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2021 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "card_p.h"

#include <gwenhywfar/debug.h>



static int _isoReadBinary(LC_CARD *card, uint32_t flags, int offset, int size, GWEN_BUFFER *buf);
static int _isoUpdateBinary(LC_CARD *card, uint32_t flags, int offset, const char *ptr, unsigned int size);
static int _isoWriteBinary(LC_CARD *card, uint32_t flags, int offset, const char *ptr, unsigned int size);
static int _isoEraseBinary(LC_CARD *card, uint32_t flags, int offset, unsigned int size);
static int _isoReadRecord(LC_CARD *card, uint32_t flags, int recNum, GWEN_BUFFER *buf);
static int _isoWriteRecord(LC_CARD *card, uint32_t flags, int recNum, const char *ptr, unsigned int size);
static int _isoUpdateRecord(LC_CARD *card, uint32_t flags, int recNum, const char *ptr, unsigned int size);
static int _isoAppendRecord(LC_CARD *card, uint32_t flags, const char *ptr, unsigned int size);

static int _isoVerifyPin(LC_CARD *card, uint32_t flags, const LC_PININFO *pi,
                                     const unsigned char *ptr, unsigned int size, int *triesLeft);
static int _isoPerformVerification(LC_CARD *card, uint32_t flags, const LC_PININFO *pi, int *triesLeft);
static int _isoModifyPin(LC_CARD *card,
                         uint32_t flags,
                         const LC_PININFO *pi,
                         const unsigned char *oldptr, unsigned int oldsize,
                         const unsigned char *newptr, unsigned int newsize,
                         int *triesLeft);
static int _isoPerformModification(LC_CARD *card, uint32_t flags, const LC_PININFO *pi, int *triesLeft);

static int _isoManageSe(LC_CARD *card, int tmpl, int kids, int kidp, int ar);
static int _isoEncipher(LC_CARD *card, const char *ptr, unsigned int size, GWEN_BUFFER *codeBuf);
static int _isoDecipher(LC_CARD *card, const char *ptr, unsigned int size, GWEN_BUFFER *plainBuf);




int _isoReadBinary(LC_CARD *card, uint32_t flags, int offset, int size, GWEN_BUFFER *buf)
{
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  int res;
  unsigned int bs;
  const void *p;

  DBG_INFO(LC_LOGDOMAIN, "Reading binary %04x:%04x", offset, size);

  if (flags & LC_CARD_ISO_FLAGS_EFID_MASK) {
    if (offset>255) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Offset too high when implicitly selecting EF "
                "(%u)", flags);
      return GWEN_ERROR_INVALID;
    }
    /* modify offset: highbyte is p1, lowbyte is p2 */
    offset|=0x8000;
    offset|=((flags & LC_CARD_ISO_FLAGS_EFID_MASK)<<8);
  }

  dbReq=GWEN_DB_Group_new("request");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "offset", offset);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "lr", size);

  res=LC_Card_ExecCommand(card, "IsoReadBinary", dbReq, dbResp);
  if (res<0) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    return res;
  }

  /* successful */
  if (buf) {
    p=GWEN_DB_GetBinValue(dbResp, "response/data", 0, 0, 0, &bs);
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



int _isoUpdateBinary(LC_CARD *card, uint32_t flags, int offset, const char *ptr, unsigned int size)
{
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  int res;

  DBG_DEBUG(LC_LOGDOMAIN, "Writing binary %04x:%04x", offset, size);

  if (flags & LC_CARD_ISO_FLAGS_EFID_MASK) {
    if (offset>255) {
      DBG_ERROR(LC_LOGDOMAIN, "Offset too high when implicitly selecting EF (%u)", flags);
      return GWEN_ERROR_INVALID;
    }
    /* modify offset: highbyte is p1, lowbyte is p2 */
    offset|=0x8000;
    offset|=((flags & LC_CARD_ISO_FLAGS_EFID_MASK)<<8);
  }

  dbReq=GWEN_DB_Group_new("request");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "offset", offset);
  if (ptr && size)
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "data", ptr, size);

  res=LC_Card_ExecCommand(card, "IsoUpdateBinary", dbReq, dbResp);
  if (res<0) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    return res;
  }

  /* successful */
  GWEN_DB_Group_free(dbResp);
  GWEN_DB_Group_free(dbReq);
  return res;
}



int _isoWriteBinary(LC_CARD *card, uint32_t flags, int offset, const char *ptr, unsigned int size)
{
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  int res;

  DBG_DEBUG(LC_LOGDOMAIN, "Writing binary %04x:%04x", offset, size);

  if (flags & LC_CARD_ISO_FLAGS_EFID_MASK) {
    if (offset>255) {
      DBG_ERROR(LC_LOGDOMAIN, "Offset too high when implicitly selecting EF (%u)", flags);
      return GWEN_ERROR_INVALID;
    }
    /* modify offset: highbyte is p1, lowbyte is p2 */
    offset|=0x8000;
    offset|=((flags & LC_CARD_ISO_FLAGS_EFID_MASK)<<8);
  }

  dbReq=GWEN_DB_Group_new("request");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "offset", offset);
  if (ptr && size)
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "data", ptr, size);

  res=LC_Card_ExecCommand(card, "IsoWriteBinary", dbReq, dbResp);
  if (res<0) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    return res;
  }

  /* successful */
  GWEN_DB_Group_free(dbResp);
  GWEN_DB_Group_free(dbReq);
  return res;
}



int _isoEraseBinary(LC_CARD *card, uint32_t flags, int offset, unsigned int size)
{
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  int res;

  DBG_DEBUG(LC_LOGDOMAIN, "Erasing binary %04x:%04x", offset, size);

  if (flags & LC_CARD_ISO_FLAGS_EFID_MASK) {
    if (offset>255) {
      DBG_ERROR(LC_LOGDOMAIN, "Offset too high when implicitly selecting EF (%u)", flags);
      return GWEN_ERROR_INVALID;
    }
    /* modify offset: highbyte is p1, lowbyte is p2 */
    offset|=0x8000;
    offset|=((flags & LC_CARD_ISO_FLAGS_EFID_MASK)<<8);
  }

  dbReq=GWEN_DB_Group_new("request");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "offset", offset);
  if (size!=0)
    GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "len", size);

  res=LC_Card_ExecCommand(card, "IsoEraseBinary", dbReq, dbResp);
  if (res<0) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    return res;
  }

  /* successful */
  GWEN_DB_Group_free(dbResp);
  GWEN_DB_Group_free(dbReq);
  return res;
}



int _isoReadRecord(LC_CARD *card, uint32_t flags, int recNum, GWEN_BUFFER *buf)
{
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  int res;
  unsigned int bs;
  const void *p;
  unsigned char p2;

  p2=(flags & LC_CARD_ISO_FLAGS_EFID_MASK)<<3;
  if ((flags & LC_CARD_ISO_FLAGS_RECSEL_MASK)!=
      LC_CARD_ISO_FLAGS_RECSEL_GIVEN) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid flags %u (only RECSEL_GIVEN is allowed)", flags);
    return GWEN_ERROR_INVALID;
  }
  p2|=0x04;

  dbReq=GWEN_DB_Group_new("request");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "recNum", recNum);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "p2", p2);

  res=LC_Card_ExecCommand(card, "IsoReadRecord", dbReq, dbResp);
  if (res<0) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    return res;
  }

  /* successful */
  if (buf) {
    p=GWEN_DB_GetBinValue(dbResp, "response/data", 0, 0, 0, &bs);
    if (p && bs)
      GWEN_Buffer_AppendBytes(buf, p, bs);
    else {
      DBG_WARN(LC_LOGDOMAIN, "No data in response");
    }
  }

  GWEN_DB_Group_free(dbResp);
  GWEN_DB_Group_free(dbReq);
  return res;
}



int _isoWriteRecord(LC_CARD *card, uint32_t flags, int recNum, const char *ptr, unsigned int size)
{
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  int res;

  dbReq=GWEN_DB_Group_new("request");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "recNum", recNum);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "p2",
                      ((flags & LC_CARD_ISO_FLAGS_RECSEL_MASK)>>5) |
                      ((flags & LC_CARD_ISO_FLAGS_EFID_MASK)<<3));
  if (ptr && size)
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "data", ptr, size);

  res=LC_Card_ExecCommand(card, "IsoWriteRecord", dbReq, dbResp);
  if (res<0) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    return res;
  }

  GWEN_DB_Group_free(dbResp);
  GWEN_DB_Group_free(dbReq);
  return res;

}



int _isoUpdateRecord(LC_CARD *card, uint32_t flags, int recNum, const char *ptr, unsigned int size)
{
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  int res;

  dbReq=GWEN_DB_Group_new("request");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "recNum", recNum);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "p2",
                      ((flags & LC_CARD_ISO_FLAGS_RECSEL_MASK)>>5) |
                      ((flags & LC_CARD_ISO_FLAGS_EFID_MASK)<<3));
  if (ptr && size)
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "data", ptr, size);

  res=LC_Card_ExecCommand(card, "IsoUpdateRecord", dbReq, dbResp);
  if (res<0) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    return res;
  }

  GWEN_DB_Group_free(dbResp);
  GWEN_DB_Group_free(dbReq);
  return res;
}



int _isoAppendRecord(LC_CARD *card, uint32_t flags, const char *ptr, unsigned int size)
{
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  int res;

  dbReq=GWEN_DB_Group_new("request");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "p2",
                      (flags & LC_CARD_ISO_FLAGS_EFID_MASK)<<3);

  if (ptr && size)
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "data", ptr, size);

  res=LC_Card_ExecCommand(card, "IsoAppendRecord", dbReq, dbResp);
  if (res<0) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    return res;
  }

  GWEN_DB_Group_free(dbResp);
  GWEN_DB_Group_free(dbReq);
  return res;
}



int _isoVerifyPin(LC_CARD *card, uint32_t flags, const LC_PININFO *pi,
                  const unsigned char *ptr, unsigned int size,
                  int *triesLeft)
{
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  GWEN_DB_NODE *dbT;
  int res;
  const char *cmd;

  if (triesLeft)
    *triesLeft=-1;

  switch (LC_PinInfo_GetEncoding(pi)) {
  case GWEN_Crypt_PinEncoding_Bin:
    cmd="IsoVerifyPin_Bin";
    break;
  case GWEN_Crypt_PinEncoding_Bcd:
    cmd="IsoVerifyPin_Bcd";
    break;
  case GWEN_Crypt_PinEncoding_Ascii:
    cmd="IsoVerifyPin_Ascii";
    break;
  case GWEN_Crypt_PinEncoding_FPin2:
    cmd="IsoVerifyPin_Fpin2";
    break;
  default:
    DBG_ERROR(LC_LOGDOMAIN, "Unhandled pin encoding \"%s\"",
              GWEN_Crypt_PinEncoding_toString(LC_PinInfo_GetEncoding(pi)));
    return GWEN_ERROR_INVALID;
  }

  dbReq=GWEN_DB_Group_new("request");
  dbResp=GWEN_DB_Group_new("response");
  dbT=GWEN_DB_GetGroup(dbReq, GWEN_DB_FLAGS_OVERWRITE_GROUPS, "pinInfo");
  assert(dbT);
  LC_PinInfo_toDb(pi, dbT);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS, "pid", LC_PinInfo_GetId(pi));

  if (ptr && size)
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "pin", ptr, size);

  res=LC_Card_ExecCommand(card, cmd, dbReq, dbResp);
  if (res<0) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    if (triesLeft) {
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



int _isoModifyPin(LC_CARD *card,
                  uint32_t flags,
                  const LC_PININFO *pi,
                  const unsigned char *oldptr,
                  unsigned int oldsize,
                  const unsigned char *newptr,
                  unsigned int newsize,
                  int *triesLeft)
{
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  GWEN_DB_NODE *dbT;
  int res;
  const char *cmd;

  if (triesLeft)
    *triesLeft=-1;

  switch (LC_PinInfo_GetEncoding(pi)) {
  case GWEN_Crypt_PinEncoding_Bin:
    cmd="IsoModifyPin_Bin";
    break;
  case GWEN_Crypt_PinEncoding_Bcd:
    cmd="IsoModifyPin_Bcd";
    break;
  case GWEN_Crypt_PinEncoding_Ascii:
    cmd="IsoModifyPin_Ascii";
    break;
  case GWEN_Crypt_PinEncoding_FPin2:
    cmd="IsoModifyPin_Fpin2";
    break;
  default:
    DBG_ERROR(LC_LOGDOMAIN, "Unhandled pin encoding \"%s\"",
              GWEN_Crypt_PinEncoding_toString(LC_PinInfo_GetEncoding(pi)));
    return GWEN_ERROR_INVALID;
  }

  dbReq=GWEN_DB_Group_new("request");
  dbResp=GWEN_DB_Group_new("response");
  dbT=GWEN_DB_GetGroup(dbReq, GWEN_DB_FLAGS_OVERWRITE_GROUPS, "pinInfo");
  assert(dbT);
  LC_PinInfo_toDb(pi, dbT);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS, "pid", LC_PinInfo_GetId(pi));

  if (oldptr && oldsize)
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "oldpin", oldptr, oldsize);

  if (newptr && newsize)
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "newpin", newptr, newsize);

  res=LC_Card_ExecCommand(card, cmd, dbReq, dbResp);
  if (res<0) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    if (triesLeft) {
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



int _isoPerformVerification(LC_CARD *card, uint32_t flags, const LC_PININFO *pi, int *triesLeft)
{
  GWEN_DB_NODE *dbReq=0;
  GWEN_DB_NODE *dbResp;
  GWEN_DB_NODE *dbT;
  int res;
  const char *cmd;

  if (triesLeft)
    *triesLeft=-1;

  switch (LC_PinInfo_GetEncoding(pi)) {
  case GWEN_Crypt_PinEncoding_Bin:
    cmd="IsoPerformVerification_Bin";
    break;
  case GWEN_Crypt_PinEncoding_Bcd:
    cmd="IsoPerformVerification_Bcd";
    break;
  case GWEN_Crypt_PinEncoding_Ascii:
    cmd="IsoPerformVerification_Ascii";
    break;
  case GWEN_Crypt_PinEncoding_FPin2:
    cmd="IsoPerformVerification_Fpin2";
    break;
  default:
    DBG_ERROR(LC_LOGDOMAIN, "Unhandled pin encoding \"%s\"",
              GWEN_Crypt_PinEncoding_toString(LC_PinInfo_GetEncoding(pi)));
    return GWEN_ERROR_INVALID;
  }

  dbReq=GWEN_DB_Group_new("request");
  dbResp=GWEN_DB_Group_new("response");
  dbT=GWEN_DB_GetGroup(dbReq, GWEN_DB_FLAGS_OVERWRITE_GROUPS, "pinInfo");
  assert(dbT);
  LC_PinInfo_toDb(pi, dbT);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS, "pid", LC_PinInfo_GetId(pi));

  res=LC_Card_ExecCommand(card, cmd, dbReq, dbResp);
  DBG_DEBUG(LC_LOGDOMAIN, "ExecCommand returned %d", res);
  if (res<0) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    if (triesLeft) {
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



int _isoPerformModification(LC_CARD *card, uint32_t flags, const LC_PININFO *pi, int *triesLeft)
{
  GWEN_DB_NODE *dbReq=0;
  GWEN_DB_NODE *dbResp;
  GWEN_DB_NODE *dbT;
  int res;
  const char *cmd;

  if (triesLeft)
    *triesLeft=-1;

  switch (LC_PinInfo_GetEncoding(pi)) {
  case GWEN_Crypt_PinEncoding_Bin:
    cmd="IsoPerformModification_Bin";
    break;
  case GWEN_Crypt_PinEncoding_Bcd:
    cmd="IsoPerformModification_Bcd";
    break;
  case GWEN_Crypt_PinEncoding_Ascii:
    cmd="IsoPerformModification_Ascii";
    break;
  case GWEN_Crypt_PinEncoding_FPin2:
    cmd="IsoPerformModification_Fpin2";
    break;
  default:
    DBG_ERROR(LC_LOGDOMAIN, "Unhandled pin encoding \"%s\"",
              GWEN_Crypt_PinEncoding_toString(LC_PinInfo_GetEncoding(pi)));
    return GWEN_ERROR_INVALID;
  }

  dbReq=GWEN_DB_Group_new("request");
  dbResp=GWEN_DB_Group_new("response");
  dbT=GWEN_DB_GetGroup(dbReq, GWEN_DB_FLAGS_OVERWRITE_GROUPS, "pinInfo");
  assert(dbT);
  LC_PinInfo_toDb(pi, dbT);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS, "pid", LC_PinInfo_GetId(pi));

  res=LC_Card_ExecCommand(card, cmd, dbReq, dbResp);
  if (res<0) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    if (triesLeft) {
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



int _isoManageSe(LC_CARD *card, int tmpl, int kids, int kidp, int ar)
{
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  int res;
  GWEN_BUFFER *dbuf;

  assert(card);

  LC_Card_SetLastResult(card, 0, 0, 0, 0);

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

  dbReq=GWEN_DB_Group_new("request");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "template", tmpl);
  GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "data",
                      GWEN_Buffer_GetStart(dbuf), GWEN_Buffer_GetUsedBytes(dbuf));
  GWEN_Buffer_free(dbuf);

  dbResp=GWEN_DB_Group_new("response");
  res=LC_Card_ExecCommand(card, "IsoManageSE", dbReq, dbResp);
  if (res<0) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    return res;
  }
  GWEN_DB_Group_free(dbReq);
  GWEN_DB_Group_free(dbResp);
  return 0;
}



int _isoInternalAuth(LC_CARD *card, int kid, const unsigned char *ptr, unsigned int size, GWEN_BUFFER *rBuf)
{
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  int res;

  dbReq=GWEN_DB_Group_new("request");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS, "kid", kid);

  if (ptr && size)
    GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "data", ptr, size);

  res=LC_Card_ExecCommand(card, "IsoInternalAuth", dbReq, dbResp);
  if (res<0) {
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbResp);
    return res;
  }


  if (rBuf) {
    unsigned int bs;
    const void *p;

    p=GWEN_DB_GetBinValue(dbResp, "response/data", 0, 0, 0, &bs);
    if (p && bs)
      GWEN_Buffer_AppendBytes(rBuf, p, bs);
    else {
      DBG_WARN(LC_LOGDOMAIN, "No data in response");
    }
  }

  GWEN_DB_Group_free(dbResp);
  GWEN_DB_Group_free(dbReq);
  return res;
}




int _isoEncipher(LC_CARD *card, const char *ptr, unsigned int size, GWEN_BUFFER *codeBuf)
{
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbRsp;
  int res;
  const void *p;
  unsigned int bs;

  assert(card);

  /* put data */
  dbReq=GWEN_DB_Group_new("request");
  dbRsp=GWEN_DB_Group_new("response");
  GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "data", ptr, size);
  LC_Card_SetLastResult(card, 0, 0, 0, 0);
  res=LC_Card_ExecCommand(card, "IsoEncipher", dbReq, dbRsp);
  if (res<0) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbRsp);
    return res;
  }

  /* extract the encoded data */
  p=GWEN_DB_GetBinValue(dbRsp, "response/data", 0, 0, 0, &bs);
  if (!p || !bs) {
    DBG_ERROR(LC_LOGDOMAIN, "No data returned by card");
    GWEN_DB_Dump(dbRsp, 2);
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbRsp);
    return res;
  }
  GWEN_Buffer_AppendBytes(codeBuf, p, bs);
  GWEN_DB_Group_free(dbReq);
  GWEN_DB_Group_free(dbRsp);

  return 0;
}



int _isoDecipher(LC_CARD *card, const char *ptr, unsigned int size, GWEN_BUFFER *plainBuf)
{
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbRsp;
  int res;
  const void *p;
  unsigned int bs;

  assert(card);

  /* put hash */
  dbReq=GWEN_DB_Group_new("request");
  dbRsp=GWEN_DB_Group_new("response");
  GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_DEFAULT, "data", ptr, size);
  LC_Card_SetLastResult(card, 0, 0, 0, 0);
  res=LC_Card_ExecCommand(card, "IsoDecipher", dbReq, dbRsp);
  if (res<0) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbRsp);
    return res;
  }

  /* extract the decoded data */
  p=GWEN_DB_GetBinValue(dbRsp, "response/data", 0, 0, 0, &bs);
  if (!p || !bs) {
    DBG_ERROR(LC_LOGDOMAIN, "No data returned by card");
    GWEN_DB_Group_free(dbReq);
    GWEN_DB_Group_free(dbRsp);
    return res;
  }
  GWEN_Buffer_AppendBytes(plainBuf, p, bs);
  GWEN_DB_Group_free(dbReq);
  GWEN_DB_Group_free(dbRsp);

  return 0;
}







int LC_Card_IsoReadBinary(LC_CARD *card,
                          uint32_t flags,
                          int offset,
                          int size,
                          GWEN_BUFFER *buf)
{
  assert(card);
  if (card->readBinaryFn)
    return card->readBinaryFn(card, flags, offset, size, buf);
  else
    return _isoReadBinary(card, flags, offset, size, buf);
}



int LC_Card_IsoWriteBinary(LC_CARD *card,
                                        uint32_t flags,
                                        int offset,
                                        const char *ptr,
                                        unsigned int size)
{
  assert(card);
  if (card->writeBinaryFn)
    return card->writeBinaryFn(card, flags, offset, ptr, size);
  else
    return _isoWriteBinary(card, flags, offset, ptr, size);
}



int LC_Card_IsoUpdateBinary(LC_CARD *card,
                                         uint32_t flags,
                                         int offset,
                                         const char *ptr,
                                         unsigned int size)
{
  assert(card);
  if (card->updateBinaryFn)
    return card->updateBinaryFn(card, flags, offset, ptr, size);
  else
    return _isoUpdateBinary(card, flags, offset, ptr, size);
}




int LC_Card_IsoEraseBinary(LC_CARD *card,
                                        uint32_t flags,
                                        int offset,
                                        unsigned int size)
{
  assert(card);
  if (card->eraseBinaryFn)
    return card->eraseBinaryFn(card, flags, offset, size);
  else
    return _isoEraseBinary(card, flags, offset, size);
}



int LC_Card_IsoReadRecord(LC_CARD *card,
                                       uint32_t flags,
                                       int recNum,
                                       GWEN_BUFFER *buf)
{
  assert(card);
  if (card->readRecordFn)
    return card->readRecordFn(card, flags, recNum, buf);
  else
    return _isoReadRecord(card, flags, recNum, buf);
}



int LC_Card_IsoWriteRecord(LC_CARD *card,
                                        uint32_t flags,
                                        int recNum,
                                        const char *ptr,
                                        unsigned int size)
{
  assert(card);
  if (card->writeRecordFn)
    return card->writeRecordFn(card, flags, recNum, ptr, size);
  else
    return _isoWriteRecord(card, flags, recNum, ptr, size);
}



int LC_Card_IsoAppendRecord(LC_CARD *card,
                                         uint32_t flags,
                                         const char *ptr,
                                         unsigned int size)
{
  assert(card);
  if (card->appendRecordFn)
    return card->appendRecordFn(card, flags, ptr, size);
  else
    return _isoAppendRecord(card, flags, ptr, size);
}




int LC_Card_IsoUpdateRecord(LC_CARD *card,
                                         uint32_t flags,
                                         int recNum,
                                         const char *ptr,
                                         unsigned int size)
{
  assert(card);
  if (card->updateRecordFn)
    return card->updateRecordFn(card, flags, recNum, ptr, size);
  else
    return _isoUpdateRecord(card, flags, recNum, ptr, size);
}



int LC_Card_IsoVerifyPin(LC_CARD *card,
                                      uint32_t flags,
                                      const LC_PININFO *pi,
                                      const unsigned char *ptr,
                                      unsigned int size,
                                      int *triesLeft)
{
  assert(card);
  if (card->verifyPinFn)
    return card->verifyPinFn(card, flags, pi, ptr, size,
                             triesLeft);
  else
    return _isoVerifyPin(card, flags, pi, ptr, size,
                                 triesLeft);
}



int LC_Card_IsoModifyPin(LC_CARD *card,
                                      uint32_t flags,
                                      const LC_PININFO *pi,
                                      const unsigned char *oldptr,
                                      unsigned int oldsize,
                                      const unsigned char *newptr,
                                      unsigned int newsize,
                                      int *triesLeft)
{
  assert(card);
  if (card->modifyPinFn)
    return card->modifyPinFn(card, flags, pi,
                             oldptr, oldsize,
                             newptr, newsize,
                             triesLeft);
  else
    return _isoModifyPin(card, flags, pi,
                                 oldptr, oldsize,
                                 newptr, newsize,
                                 triesLeft);
}



int LC_Card_IsoPerformVerification(LC_CARD *card,
                                                uint32_t flags,
                                                const LC_PININFO *pi,
                                                int *triesLeft)
{
  assert(card);
  if (card->performVerificationFn)
    return card->performVerificationFn(card, flags, pi,
                                       triesLeft);
  else
    return _isoPerformVerification(card, flags, pi,
                                           triesLeft);
}



int LC_Card_IsoPerformModification(LC_CARD *card,
                                                uint32_t flags,
                                                const LC_PININFO *pi,
                                                int *triesLeft)
{
  assert(card);
  if (card->performModificationFn)
    return card->performModificationFn(card, flags, pi,
                                       triesLeft);
  else
    return _isoPerformModification(card, flags, pi,
                                           triesLeft);
}



int LC_Card_IsoManageSe(LC_CARD *card,
                                     int tmpl, int kids, int kidp, int ar)
{
  assert(card);
  if (card->manageSeFn)
    return card->manageSeFn(card, tmpl, kids, kidp, ar);
  else
    return _isoManageSe(card, tmpl, kids, kidp, ar);
}



int LC_Card_IsoEncipher(LC_CARD *card,
                                     const char *ptr,
                                     unsigned int size,
                                     GWEN_BUFFER *codeBuf)
{
  assert(card);
  if (card->encipherFn)
    return card->encipherFn(card, ptr, size, codeBuf);
  else
    return _isoEncipher(card, ptr, size, codeBuf);
}



int LC_Card_IsoDecipher(LC_CARD *card,
                                     const char *ptr,
                                     unsigned int size,
                                     GWEN_BUFFER *plainBuf)
{
  assert(card);
  if (card->decipherFn)
    return card->decipherFn(card, ptr, size, plainBuf);
  else
    return _isoDecipher(card, ptr, size, plainBuf);
}



int LC_Card_IsoSign(LC_CARD *card,
                                 const char *ptr,
                                 unsigned int size,
                                 GWEN_BUFFER *sigBuf)
{
  assert(card);
  if (card->signFn)
    return card->signFn(card, ptr, size, sigBuf);
  else
    return GWEN_ERROR_NOT_SUPPORTED;
}



int LC_Card_IsoVerify(LC_CARD *card,
                                   const char *dptr,
                                   unsigned int dsize,
                                   const char *sigptr,
                                   unsigned int sigsize)
{
  assert(card);
  if (card->verifyFn)
    return card->verifyFn(card, dptr, dsize, sigptr, sigsize);
  else
    return GWEN_ERROR_NOT_SUPPORTED;
}



int LC_Card_IsoInternalAuth(LC_CARD *card,
                                         int kid,
                                         const unsigned char *ptr,
                                         unsigned int size,
                                         GWEN_BUFFER *rBuf)
{
  assert(card);
  if (card->internalAuthFn)
    return card->internalAuthFn(card, kid, ptr, size, rBuf);
  else
    return GWEN_ERROR_NOT_SUPPORTED;
}







void LC_Card_SetIsoReadBinaryFn(LC_CARD *card, LC_CARD_ISOREADBINARY_FN f)
{
  assert(card);
  card->readBinaryFn=f;
}



void LC_Card_SetIsoWriteBinaryFn(LC_CARD *card, LC_CARD_ISOWRITEBINARY_FN f)
{
  assert(card);
  card->writeBinaryFn=f;
}



void LC_Card_SetIsoUpdateBinaryFn(LC_CARD *card,
                                  LC_CARD_ISOUPDATEBINARY_FN f)
{
  assert(card);
  card->updateBinaryFn=f;
}



void LC_Card_SetIsoEraseBinaryFn(LC_CARD *card, LC_CARD_ISOERASEBINARY_FN f)
{
  assert(card);
  card->eraseBinaryFn=f;
}



void LC_Card_SetIsoReadRecordFn(LC_CARD *card, LC_CARD_ISOREADRECORD_FN f)
{
  assert(card);
  card->readRecordFn=f;
}



void LC_Card_SetIsoWriteRecordFn(LC_CARD *card, LC_CARD_ISOWRITERECORD_FN f)
{
  assert(card);
  card->writeRecordFn=f;
}



void LC_Card_SetIsoUpdateRecordFn(LC_CARD *card,
                                  LC_CARD_ISOUPDATERECORD_FN f)
{
  assert(card);
  card->updateRecordFn=f;
}



void LC_Card_SetIsoAppendRecordFn(LC_CARD *card,
                                  LC_CARD_ISOAPPENDRECORD_FN f)
{
  assert(card);
  card->appendRecordFn=f;
}



void LC_Card_SetIsoVerifyPinFn(LC_CARD *card, LC_CARD_ISOVERIFYPIN_FN f)
{
  assert(card);
  card->verifyPinFn=f;
}



void LC_Card_SetIsoModifyPinFn(LC_CARD *card, LC_CARD_ISOMODIFYPIN_FN f)
{
  assert(card);
  card->modifyPinFn=f;
}



void LC_Card_SetIsoPerformVerificationFn(LC_CARD *card,
                                         LC_CARD_ISOPERFORMVERIFICATION_FN f)
{
  assert(card);
  card->performVerificationFn=f;
}



void LC_Card_SetIsoPerformModificationFn(LC_CARD *card,
                                         LC_CARD_ISOPERFORMMODIFICATION_FN f)
{
  assert(card);
  card->performModificationFn=f;
}



void LC_Card_SetIsoManageSeFn(LC_CARD *card, LC_CARD_ISOMANAGESE_FN f)
{
  assert(card);
  card->manageSeFn=f;
}



void LC_Card_SetIsoSignFn(LC_CARD *card, LC_CARD_ISOSIGN_FN f)
{
  assert(card);
  card->signFn=f;
}



void LC_Card_SetIsoVerifyFn(LC_CARD *card, LC_CARD_ISOVERIFY_FN f)
{
  assert(card);
  card->verifyFn=f;
}



void LC_Card_SetIsoEncipherFn(LC_CARD *card, LC_CARD_ISOENCIPHER_FN f)
{
  assert(card);
  card->encipherFn=f;
}



void LC_Card_SetIsoDecipherFn(LC_CARD *card, LC_CARD_ISODECIPHER_FN f)
{
  assert(card);
  card->decipherFn=f;
}



void LC_Card_SetIsoInternalAuthFn(LC_CARD *card, LC_CARD_ISOINTERNALAUTH_FN f)
{
  assert(card);
  card->internalAuthFn=f;
}



