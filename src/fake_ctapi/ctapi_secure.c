/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: ctapi.c 186 2006-06-07 13:11:19Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/




int CT__fillPinInfo(LC_PININFO *pi, CTAPI_APDU *apdu) {
  GWEN_BUFFER *dbuf;
  GWEN_TLV *tlv=0;
  const unsigned char *p;
  int i;
  int j;

  if (apdu->dlen<8) {
    DBG_ERROR(CT_API_LOGDOMAIN,
              "Bad APDU: Too few data bytes in APDU (only %d bytes)",
              apdu->dlen);
    return -1;
  }

  dbuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Buffer_AppendBytes(dbuf, (const char*)apdu->data, apdu->dlen);
  GWEN_Buffer_Rewind(dbuf);

  /* find tag 0x52 */
  while(GWEN_Buffer_GetBytesLeft(dbuf)) {
    tlv=GWEN_TLV_fromBuffer(dbuf, 0);
    if (!tlv)
      break;
    if (GWEN_TLV_GetTagType(tlv)==0x52)
      break;
  } /* while */

  if (!tlv) {
    DBG_ERROR(CT_API_LOGDOMAIN, "TLV 0x52 not found");
    return -1;
  }

  if (GWEN_TLV_GetTagLength(tlv)<6) {
    DBG_ERROR(CT_API_LOGDOMAIN, "TLV 0x52 too small");
    GWEN_TLV_free(tlv);
    return -1;
  }
  p=(const unsigned char*)GWEN_TLV_GetTagData(tlv);

  /* extract pin id */
  LC_PinInfo_SetId(pi, (unsigned int)p[5]);

  /* extract pin encoding, pin length etc */
  i=(unsigned int)p[0];

  /* extract pin length */
  j=(i>>5) & 0x7;
  if (j)
    LC_PinInfo_SetMinLength(pi, j);

  switch(i & 0x3) {
  case 0x00:
    DBG_DEBUG(CT_API_LOGDOMAIN, "Pin encoding is BCD");
    LC_PinInfo_SetEncoding(pi, GWEN_Crypt_PinEncoding_Bcd);
    if (GWEN_TLV_GetTagLength(tlv)>8) {
      /* special case: there are template bytes in the command, check
       * whether is is hex 0x2Y with Y!=0
       */
      j=(unsigned int)p[7];
      if ((j & 0xf0)==0x20 && (j & 0x0f)!=0x00) {
        /* now check whether pattern is 0xff */
        j=(unsigned int)p[8];
        if (j==0xff) {
          DBG_ERROR(0, "Pin Encoding really is FPIN2");
	  LC_PinInfo_SetEncoding(pi, GWEN_Crypt_PinEncoding_FPin2);
        }
      }
    }
    break;

  case 0x01:
    DBG_DEBUG(CT_API_LOGDOMAIN, "Pin encoding is ASCII");
    LC_PinInfo_SetEncoding(pi, GWEN_Crypt_PinEncoding_Ascii);
    j=(int)p[6];
    if (j==0 || j>CTAPI_DEF_PINMAXLEN)
      j=CTAPI_DEF_PINMAXLEN;
    LC_PinInfo_SetMaxLength(pi, j);
    break;

  case 0x02:
    DBG_DEBUG(CT_API_LOGDOMAIN, "Pin encoding is FPIN2");
    LC_PinInfo_SetEncoding(pi, GWEN_Crypt_PinEncoding_FPin2);
    LC_PinInfo_SetMaxLength(pi, 14);
    break;

  default:
    DBG_ERROR(CT_API_LOGDOMAIN, "Unknown pin encoding \"%02x\"", i);
    GWEN_TLV_free(tlv);
    return -1;
  }

  GWEN_TLV_free(tlv);
  return 0;
}




char CT__secureVerify(CTAPI_CONTEXT *ctx,
                      unsigned char *dad,
                      unsigned char *sad,
                      CTAPI_APDU *apdu,
                      unsigned short *lenr,
                      unsigned char *response){
  LC_CLIENT_RESULT res;
  unsigned char *p;
  int j;
  unsigned char uc;
  unsigned char sw1, sw2;
  LC_PININFO *pi;
  int triesLeft=0;

  DBG_ERROR(CT_API_LOGDOMAIN, "SecureVerify");

  uc=*sad;
  *sad=*dad;
  *dad=uc;

  if (ctx->cardType==0 ||
      !(LC_Card_GetReaderFlags(ctx->card) & LC_READER_FLAGS_KEYPAD)) {
    DBG_ERROR(CT_API_LOGDOMAIN,
              "SecureVerify not available (card type: %s)",
              (ctx->cardType)?(ctx->cardType):"(none)");
    response[0]=0x6d; /* not supported */
    response[1]=0x00;
    *lenr=2;
    return CT_API_RV_OK;
  }

  /* extract pin information */
  pi=LC_PinInfo_new();
  if (CT__fillPinInfo(pi, apdu)) {
    LC_PinInfo_free(pi);
    return CT_API_RV_ERR_INVALID;
  }

  res=LC_Card_IsoPerformVerification(ctx->card,
				     0,
				     pi,
				     &triesLeft);
  LC_PinInfo_free(pi);

  switch(res) {

  case LC_Client_ResultOk:
    sw1=0x90;
    sw2=0x00;
    break;

  case LC_Client_ResultWait:
    sw1=0x64;
    sw2=0x00;
    break;

  case LC_Client_ResultIpcError:
    DBG_ERROR(CT_API_LOGDOMAIN, "IPC error");
    return CT_API_RV_ERR_HOST;

  case LC_Client_ResultCmdError:
    sw1=LC_Card_GetLastSW1(ctx->card);
    sw2=LC_Card_GetLastSW2(ctx->card);
    break;

  case LC_Client_ResultAborted:
    sw1=0x64;
    sw2=0x01;
    break;

  case LC_Client_ResultIoError:
    CT__closeCard(ctx);
    sw1=0x65;  /* Memory failure (TODO: select better code) */
    sw2=0x81;
    break;

  default:
    DBG_ERROR(CT_API_LOGDOMAIN, "CT error (%d)", res);
    return CT_API_RV_ERR_CT;
  }

  *dad=*sad;
  *sad=CT_API_AD_CT;
  p=response;
  j=0;
  *(p++)=sw1;
  j++;
  *(p++)=sw2;
  j++;
  *lenr=j;
  return CT_API_RV_OK;
}



char CT__secureModify(CTAPI_CONTEXT *ctx,
                      unsigned char *dad,
                      unsigned char *sad,
                      CTAPI_APDU *apdu,
                      unsigned short *lenr,
                      unsigned char *response){
  LC_CLIENT_RESULT res;
  unsigned char *p;
  int j=0;
  unsigned char uc;
  unsigned char sw1, sw2;
  LC_PININFO *pi;
  int triesLeft=0;

  DBG_ERROR(CT_API_LOGDOMAIN, "SecureVerify");

  uc=*sad;
  *sad=*dad;
  *dad=uc;

  if (ctx->cardType==0 ||
      !(LC_Card_GetReaderFlags(ctx->card) & LC_READER_FLAGS_KEYPAD)) {
    DBG_ERROR(CT_API_LOGDOMAIN,
              "SecureVerify not available");
    response[0]=0x6d; /* not supported */
    response[1]=0x00;
    *lenr=2;
    return CT_API_RV_OK;
  }

  /* extract pin information */
  pi=LC_PinInfo_new();
  if (CT__fillPinInfo(pi, apdu)) {
    LC_PinInfo_free(pi);
    return CT_API_RV_ERR_INVALID;
  }

  res=LC_Card_IsoPerformModification(ctx->card,
				     0,
				     pi,
				     &triesLeft);
  LC_PinInfo_free(pi);

  switch(res) {

  case LC_Client_ResultOk:
    sw1=0x90;
    sw2=0x00;
    break;

  case LC_Client_ResultWait:
    sw1=0x64;
    sw2=0x00;
    break;

  case LC_Client_ResultIpcError:
    DBG_ERROR(CT_API_LOGDOMAIN, "IPC error");
    return CT_API_RV_ERR_HOST;

  case LC_Client_ResultCmdError:
    sw1=LC_Card_GetLastSW1(ctx->card);
    sw2=LC_Card_GetLastSW2(ctx->card);
    break;

  case LC_Client_ResultAborted:
    sw1=0x64;
    sw2=0x01;
    break;

  case LC_Client_ResultIoError:
    CT__closeCard(ctx);
    sw1=0x65;  /* Memory failure (TODO: select better code) */
    sw2=0x81;
    break;

  default:
    DBG_ERROR(CT_API_LOGDOMAIN, "CT error (%d)", res);
    return CT_API_RV_ERR_CT;
  }

  *dad=*sad;
  *sad=CT_API_AD_CT;
  p=response;
  j=0;
  *(p++)=sw1;
  j++;
  *(p++)=sw2;
  j++;
  *lenr=j;
  return CT_API_RV_OK;


}
