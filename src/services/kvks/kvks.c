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
#undef BUILDING_LIBCHIPCARD2_DLL

#include "kvks_p.h"
#include "kvkscard.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/gwentime.h>
#include <gwenhywfar/inetsocket.h>
#include <chipcard2-client/client/client.h>
#include <chipcard2-client/client/tlv.h>

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>



GWEN_INHERIT(LC_CLIENT, SERVICE_KVK)



LC_CLIENT *ServiceKVK_new(int argc, char **argv){
  SERVICE_KVK *kvks;
  LC_CLIENT *cl;

  cl=LC_Service_new(argc, argv);
  if (!cl) {
    DBG_ERROR(0, "Could not create service, aborting");
    return 0;
  }
  GWEN_NEW_OBJECT(SERVICE_KVK, kvks);
  GWEN_INHERIT_SETDATA(LC_CLIENT, SERVICE_KVK,
                       cl, kvks,
                       ServiceKVK_freeData);
  LC_Service_SetCommandFn(cl, ServiceKVK_Command);
  LC_Service_SetWorkFn(cl, ServiceKVK_Work);
  kvks->cards=LC_Card_List2_new();
  return cl;
}



void ServiceKVK_freeData(void *bp, void *p) {
  SERVICE_KVK *kvks;

  kvks=(SERVICE_KVK*)p;
  LC_Card_List2_freeAll(kvks->cards);
  GWEN_FREE_OBJECT(kvks);
}



int ServiceKVK_Start(LC_CLIENT *cl){
  SERVICE_KVK *kvks;
  LC_CLIENT_RESULT res;

  assert(cl);
  kvks=GWEN_INHERIT_GETDATA(LC_CLIENT, SERVICE_KVK, cl);
  assert(kvks);

  /* send status report to server */
  if (LC_Service_Connect(cl, "OK", "Service started")) {
    DBG_ERROR(0, "Error communicating with the server");
    return -1;
  }
  DBG_NOTICE(0, "Connected.");

  /* start to wait for inserted cards */
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(0, "Could not start to wait for cards");
    return -1;
  }

  return 0;
}



const char *ServiceKVK_GetErrorText(LC_CLIENT *cl, GWEN_TYPE_UINT32 err){
  const char *s;

  switch(err) {
  case SERVICE_KVK_ERROR_UNKNOWN_COMMAND:
    s="Unknown command";
    break;
  default:
    s="Unknown error";
    break;
  } /* switch */

  return s;
}



GWEN_TYPE_UINT32 ServiceKVK_Command(LC_CLIENT *cl,
                                    LC_SERVICECLIENT *scl,
                                    GWEN_DB_NODE *dbRequest,
                                    GWEN_DB_NODE *dbResponse) {
  DBG_ERROR(0, "This service does not take commands");
  return SERVICE_KVK_ERROR_UNKNOWN_COMMAND;
}



int ServiceKVK_NewCard(LC_CLIENT *cl, LC_CARD *cd) {
  int rv;
  SERVICE_KVK *kvks;

  assert(cl);
  kvks=GWEN_INHERIT_GETDATA(LC_CLIENT, SERVICE_KVK, cl);
  assert(kvks);

  if (strcasecmp(LC_Card_GetCardType(cd), "memory")!=0) {
    DBG_INFO(0, "Not a memory card");
    return 1;
  }

  rv=KVKSCard_ExtendCard(cd);
  if (rv) {
    DBG_WARN(0, "Could not extend card");
  }
  return 0;
}



GWEN_TYPE_UINT32 ServiceKVK_SendReadBinary(LC_CLIENT *cl, LC_CARD *cd,
                                           int offset, int size) {
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rqid;

  DBG_INFO(LC_LOGDOMAIN, "Reading binary %04x:%04x", offset, size);

  dbReq=GWEN_DB_Group_new("ReadBinary");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "offset", offset);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_DEFAULT,
                      "lr", size);
  rqid=LC_Client_SendExecCommand(cl, cd, dbReq);
  return rqid;
}



LC_CLIENT_RESULT ServiceKVK_CheckReadBinary(LC_CLIENT *cl, LC_CARD *cd) {
  SERVICE_KVK *kvks;
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  unsigned int bs;
  const void *p;

  assert(cl);
  kvks=GWEN_INHERIT_GETDATA(LC_CLIENT, SERVICE_KVK, cl);
  assert(kvks);

  dbRsp=GWEN_DB_Group_new("response");

  res=LC_Client_CheckExecCommand(cl, KVKSCard_GetCurrentRequest(cd), dbRsp);
  if (res!=LC_Client_ResultOk) {
    if (res!=LC_Client_ResultWait) {
      DBG_ERROR(0, "Error response for request \"readBinary\"");
    }
    GWEN_DB_Group_free(dbRsp);
    return res;
  }

  p=GWEN_DB_GetBinValue(dbRsp,
                        "command/response/data",
                        0,
                        0, 0,
                        &bs);
  if (p && bs) {
    GWEN_Buffer_AppendBytes(KVKSCard_GetBuffer(cd), p, bs);
  }
  else {
    DBG_ERROR(0, "No data in response");
    GWEN_DB_Group_free(dbRsp);
    return LC_Client_ResultDataError;
  }
  GWEN_DB_Group_free(dbRsp);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT ServiceKVK_CalcTagSize(LC_CLIENT *cl, LC_CARD *cd,
                                        unsigned int *tagSize) {
  const unsigned char *p;
  GWEN_BUFFER *mbuf;
  unsigned int size;
  unsigned int pos;
  unsigned int j;

  mbuf=KVKSCard_GetBuffer(cd);
  assert(mbuf);

  GWEN_Buffer_Rewind(mbuf);
  p=GWEN_Buffer_GetStart(mbuf);
  pos=0;
  size=GWEN_Buffer_GetBytesLeft(mbuf);

  /* get tag type */
  DBG_DEBUG(0, "Determining card data length");
  if (size<2) {
    DBG_ERROR(0, "Too few bytes for BER-TLV");
    return LC_Client_ResultDataError;
  }
  j=(unsigned char)(p[pos]);
  if ((j & 0x1f)==0x1f) {
    pos++;
    if (pos>=size) {
      DBG_ERROR(0, "Too few bytes");
      return LC_Client_ResultDataError;
    }
    j=(unsigned char)(p[pos]);
  }
  else
    j&=0x1f;

  /* get length */
  pos++;
  if (pos>=size) {
    DBG_ERROR(0, "Too few bytes");
    return LC_Client_ResultDataError;
  }
  j=(unsigned char)(p[pos]);
  if (j & 0x80) {
    if (j==0x81) {
      pos++;
      if (pos>=size) {
        DBG_ERROR(0, "Too few bytes");
        return LC_Client_ResultDataError;
      }
      j=(unsigned char)(p[pos]);
    } /* 0x81 */
    else if (j==0x82) {
      if (pos+1>=size) {
        DBG_ERROR(0, "Too few bytes");
        return LC_Client_ResultDataError;
      }
      pos++;
      j=((unsigned char)(p[pos]))<<8;
      pos++;
      j+=(unsigned char)(p[pos]);
    } /* 0x82 */
    else {
      DBG_ERROR(0, "Unexpected tag length modifier %02x", j);
      return LC_Client_ResultDataError;
    }
  } /* if tag length modifier */
  pos++;
  /* j now contains the tag data size, add tag header length */
  j+=pos;

  *tagSize=j;

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT ServiceKVK_HandleData(LC_CLIENT *cl, LC_CARD *cd) {
  const unsigned char *p;
  GWEN_BUFFER *mbuf;
  unsigned int size;
  unsigned int pos;
  unsigned int j;
  LC_TLV *tlv;
  GWEN_DB_NODE *dbData;
  int checksumOk;

  mbuf=KVKSCard_GetBuffer(cd);
  assert(mbuf);

  GWEN_Buffer_Rewind(mbuf);
  p=GWEN_Buffer_GetStart(mbuf);
  pos=0;
  size=GWEN_Buffer_GetBytesLeft(mbuf);

  /* get tag type */
  DBG_DEBUG(0, "Determining card data length");
  if (size<2) {
    DBG_ERROR(0, "Too few bytes for BER-TLV");
    return LC_Client_ResultDataError;
  }
  j=(unsigned char)(p[pos]);
  if ((j & 0x1f)==0x1f) {
    pos++;
    if (pos>=size) {
      DBG_ERROR(0, "Too few bytes");
      return LC_Client_ResultDataError;
    }
    j=(unsigned char)(p[pos]);
  }
  else
    j&=0x1f;

  /* get length */
  pos++;
  if (pos>=size) {
    DBG_ERROR(0, "Too few bytes");
    return LC_Client_ResultDataError;
  }
  j=(unsigned char)(p[pos]);
  if (j & 0x80) {
    if (j==0x81) {
      pos++;
      if (pos>=size) {
        DBG_ERROR(0, "Too few bytes");
        return LC_Client_ResultDataError;
      }
      j=(unsigned char)(p[pos]);
    } /* 0x81 */
    else if (j==0x82) {
      if (pos+1>=size) {
        DBG_ERROR(0, "Too few bytes");
        return LC_Client_ResultDataError;
      }
      pos++;
      j=((unsigned char)(p[pos]))<<8;
      pos++;
      j+=(unsigned char)(p[pos]);
    } /* 0x82 */
    else {
      DBG_ERROR(0, "Unexpected tag length modifier %02x", j);
      return LC_Client_ResultDataError;
    }
  } /* if tag length modifier */
  pos++;

  /* j now contains the tag data size, add tag header length */
  j+=pos;
  /* sub size of already read data */
  j-=size;
  GWEN_Buffer_IncrementPos(mbuf, size);

  /* parse data */
  DBG_DEBUG(0, "Parsing data...");
  GWEN_Buffer_Rewind(mbuf);
  dbData=GWEN_DB_Group_new("kvkData");
  if (LC_Card_ParseData(cd, "kvkdata", mbuf, dbData)) {
    DBG_ERROR(0, "Error in KVK data");
    GWEN_DB_Group_free(dbData);
    return LC_Client_ResultDataError;
  }

  /* perform checksum test */
  checksumOk=0;
  GWEN_Buffer_Rewind(mbuf);
  tlv=LC_TLV_fromBuffer(mbuf, 1);
  if (tlv) {
    if (LC_TLV_GetTagLength(tlv)) {
      GWEN_Buffer_SetPos(mbuf,
                         LC_TLV_GetTagSize(tlv)-LC_TLV_GetTagLength(tlv));

      while(GWEN_Buffer_GetBytesLeft(mbuf)) {
        LC_TLV *tlvLoop;

        tlvLoop=LC_TLV_fromBuffer(mbuf, 1);
        if (!tlvLoop) {
          DBG_ERROR(0, "Bad TLV in KVK data (pos=%d)",
                    GWEN_Buffer_GetPos(mbuf));
          LC_TLV_free(tlv);
          GWEN_DB_Group_free(dbData);
          return LC_Client_ResultDataError;
        }
        if (LC_TLV_GetTagType(tlvLoop)==0x0e) {
          unsigned int i;
          unsigned char checkSum;

          /* checksum tag */
          p=GWEN_Buffer_GetStart(mbuf);
          size=GWEN_Buffer_GetPos(mbuf);
          checkSum=0;
          for (i=0; i<size; i++)
            checkSum^=(unsigned char)(*p++);

          if (checkSum) {
            DBG_ERROR(0, "Bad checksum in kvk card (%02x)", checkSum);
            checksumOk=0;
            LC_TLV_free(tlvLoop);
            break;
          }
          DBG_NOTICE(0, "Checksum ok");
          checksumOk=1;
          LC_TLV_free(tlvLoop);
          break;
        }
        LC_TLV_free(tlvLoop);
      } /* while */
    }
    else {
      DBG_ERROR(0, "Empty card");
      GWEN_DB_Group_free(dbData);
      LC_TLV_free(tlv);
      return LC_Client_ResultDataError;
    }
    LC_TLV_free(tlv);
  }
  else {
    DBG_ERROR(0, "Internal: Bad TLVs in KVK data");
    GWEN_DB_Group_free(dbData);
    return LC_Client_ResultDataError;
  }

  if (!checksumOk) {
    DBG_ERROR(0, "Bad/missing checksum");
  }

  /* store data with the card */
  KVKSCard_SetCheckSumOk(cd, checksumOk);
  KVKSCard_SetDbCardData(cd, dbData);

  /* store data to a file */
  if (ServiceKVK_StoreCardData(cl, cd)) {
    DBG_ERROR(0, "Could not store card data");
    return LC_Client_ResultGeneric;
  }
  return LC_Client_ResultOk;
}



int ServiceKVK_HandleCard(LC_CLIENT *cl, LC_CARD *cd) {
  SERVICE_KVK *kvks;
  LC_CLIENT_RESULT res;
  GWEN_TYPE_UINT32 rqid;
  unsigned int tagLength;
  int doneSomething=0;

  assert(cl);
  kvks=GWEN_INHERIT_GETDATA(LC_CLIENT, SERVICE_KVK, cl);
  assert(kvks);

  DBG_VERBOUS(0, "Handling card");

  switch(KVKSCard_GetStatus(cd)) {
  case KVKSStatus_New:
    DBG_ERROR(0, "Taking card");
    rqid=LC_Client_SendTakeCard(cl, cd);
    if (rqid==0) {
      DBG_ERROR(0, "Could not send request \"takeCard\"");
      KVKSCard_SetCurrentRequest(cd, 0);
      KVKSCard_SetStatus(cd, KVKSStatus_Error);
      return -1;
    }
    KVKSCard_SetCurrentRequest(cd, rqid);
    KVKSCard_SetStatus(cd, KVKSStatus_Opening);
    doneSomething++;
    break;

  case KVKSStatus_Opening:
    res=LC_Client_CheckTakeCard(cl, KVKSCard_GetCurrentRequest(cd));
    if (res!=LC_Client_ResultOk) {
      if (res==LC_Client_ResultWait) {
        return 1;
      }
      else {
        DBG_ERROR(0, "Error response for request \"takeCard\"");
        LC_Client_DeleteRequest(cl, KVKSCard_GetCurrentRequest(cd));
        KVKSCard_SetCurrentRequest(cd, 0);
        KVKSCard_SetStatus(cd, KVKSStatus_Error);
        return -1;
      }
    }
    LC_Client_DeleteRequest(cl, KVKSCard_GetCurrentRequest(cd));

    DBG_ERROR(0, "Selecting CARD and APP");
    rqid=LC_Client_SendSelectCardApp(cl, cd, "kvk", "kvk");
    if (rqid==0) {
      DBG_ERROR(0, "Could not send request \"selectCardApp\"");
      KVKSCard_SetCurrentRequest(cd, 0);
      KVKSCard_SetStatus(cd, KVKSStatus_Error);
      return -1;
    }
    KVKSCard_SetCurrentRequest(cd, rqid);
    KVKSCard_SetStatus(cd, KVKSStatus_Selecting);
    doneSomething++;
    break;

  case KVKSStatus_Selecting:
    res=LC_Client_CheckSelectCardApp(cl, KVKSCard_GetCurrentRequest(cd));
    if (res!=LC_Client_ResultOk) {
      if (res==LC_Client_ResultWait) {
        return 1;
      }
      else {
        DBG_ERROR(0, "Error response for request \"selectCardApp\"");
        LC_Client_DeleteRequest(cl, KVKSCard_GetCurrentRequest(cd));
        KVKSCard_SetCurrentRequest(cd, 0);
        KVKSCard_SetStatus(cd, KVKSStatus_Error);
        return -1;
      }
    }
    LC_Client_DeleteRequest(cl, KVKSCard_GetCurrentRequest(cd));

    DBG_ERROR(0, "Reading header");
    rqid=ServiceKVK_SendReadBinary(cl, cd, 0x1e, 5);
    if (rqid==0) {
      DBG_ERROR(0, "Could not send request \"readBinary\"");
      KVKSCard_SetCurrentRequest(cd, 0);
      KVKSCard_SetStatus(cd, KVKSStatus_Error);
      return -1;
    }
    KVKSCard_SetCurrentRequest(cd, rqid);
    KVKSCard_SetStatus(cd, KVKSStatus_ReadingHeader);
    doneSomething++;
    break;

  case KVKSStatus_ReadingHeader:
    res=ServiceKVK_CheckReadBinary(cl, cd);
    if (res!=LC_Client_ResultOk) {
      if (res==LC_Client_ResultWait) {
        return 1;
      }
      else {
        DBG_ERROR(0, "Error response for request \"readBinary\"");
        LC_Client_DeleteRequest(cl, KVKSCard_GetCurrentRequest(cd));
        KVKSCard_SetCurrentRequest(cd, 0);
        KVKSCard_SetStatus(cd, KVKSStatus_Error);
        return -1;
      }
    }
    LC_Client_DeleteRequest(cl, KVKSCard_GetCurrentRequest(cd));

    DBG_ERROR(0, "Calculating data length");
    res=ServiceKVK_CalcTagSize(cl, cd, &tagLength);
    if (res!=LC_Client_ResultOk) {
      if (res==LC_Client_ResultWait) {
        return 1;
      }
      else {
        DBG_ERROR(0, "Could not calculate tag length");
        KVKSCard_SetCurrentRequest(cd, 0);
        KVKSCard_SetStatus(cd, KVKSStatus_Error);
        return -1;
      }
    }

    DBG_ERROR(0, "Reading data");
    rqid=ServiceKVK_SendReadBinary(cl, cd, 5+0x1e, tagLength-5);
    if (rqid==0) {
      DBG_ERROR(0, "Could not send request \"readBinary\"");
      KVKSCard_SetCurrentRequest(cd, 0);
      KVKSCard_SetStatus(cd, KVKSStatus_Error);
      return -1;
    }
    KVKSCard_SetCurrentRequest(cd, rqid);
    KVKSCard_SetStatus(cd, KVKSStatus_ReadingData);
    doneSomething++;
    break;

  case KVKSStatus_ReadingData:
    res=ServiceKVK_CheckReadBinary(cl, cd);
    if (res!=LC_Client_ResultOk) {
      if (res==LC_Client_ResultWait) {
        return 1;
      }
      else {
        DBG_ERROR(0, "Error response for request \"readBinary\"");
        LC_Client_DeleteRequest(cl, KVKSCard_GetCurrentRequest(cd));
        KVKSCard_SetCurrentRequest(cd, 0);
        KVKSCard_SetStatus(cd, KVKSStatus_Error);
        return -1;
      }
    }
    LC_Client_DeleteRequest(cl, KVKSCard_GetCurrentRequest(cd));

    DBG_ERROR(0, "Analyzing data");
    res=ServiceKVK_HandleData(cl, cd);
    if (res!=LC_Client_ResultOk) {
      DBG_ERROR(0, "Could not handle card data");
      KVKSCard_SetCurrentRequest(cd, 0);
      KVKSCard_SetStatus(cd, KVKSStatus_Error);
      return -1;
    }

    /* TODO: make driver beep */

    DBG_ERROR(0, "Releasing card");
    rqid=LC_Client_SendReleaseCard(cl, cd);
    if (rqid==0) {
      DBG_ERROR(0, "Could not send request \"releaseCard\"");
      KVKSCard_SetCurrentRequest(cd, 0);
      KVKSCard_SetStatus(cd, KVKSStatus_Error);
      return -1;
    }
    KVKSCard_SetCurrentRequest(cd, rqid);
    KVKSCard_SetStatus(cd, KVKSStatus_Releasing);
    doneSomething++;
    break;

  case KVKSStatus_Releasing:
    res=LC_Client_CheckReleaseCard(cl, KVKSCard_GetCurrentRequest(cd));
    if (res!=LC_Client_ResultOk) {
      if (res==LC_Client_ResultWait) {
        return 1;
      }
      else {
        DBG_ERROR(0, "Error response for request \"releaseCard\"");
        LC_Client_DeleteRequest(cl, KVKSCard_GetCurrentRequest(cd));
        KVKSCard_SetCurrentRequest(cd, 0);
        KVKSCard_SetStatus(cd, KVKSStatus_Error);
        return -1;
      }
    }
    LC_Client_DeleteRequest(cl, KVKSCard_GetCurrentRequest(cd));
    KVKSCard_SetCurrentRequest(cd, 0);
    KVKSCard_SetStatus(cd, KVKSStatus_Done);
    DBG_NOTICE(0, "Card handled");
    doneSomething++;
    break;

  case KVKSStatus_Done:
    break;

  case KVKSStatus_Error:
    DBG_ERROR(0, "Card is in ERROR status");
    break;

  default:
    break;
  } /* switch */

  return doneSomething?0:1;
}



int ServiceKVK_Work(LC_CLIENT *cl) {
  SERVICE_KVK *kvks;
  int doneSomething=0;
  LC_CARD_LIST2_ITERATOR *cit;
  LC_CARD *card;

  assert(cl);
  kvks=GWEN_INHERIT_GETDATA(LC_CLIENT, SERVICE_KVK, cl);
  assert(kvks);

  card=LC_Client_GetNextCard(cl);
  if (card) {
    int rv;

    DBG_ERROR(0, "Got a new card");

    /* new card */
    rv=ServiceKVK_NewCard(cl, card);
    if (rv) {
      if (rv<0) {
        DBG_WARN(0, "Could not handle new card");
      }
      else {
        DBG_INFO(0, "Not a KVK card");
      }
      LC_Card_free(card);
    }
    else {
      LC_Card_List2_PushBack(kvks->cards, card);
    }
  }

  /* handle all cards */
  cit=LC_Card_List2_First(kvks->cards);
  if (cit) {
    card=LC_Card_List2Iterator_Data(cit);
    while(card) {
      int rv;

      rv=ServiceKVK_HandleCard(cl, card);
      if (rv==0)
        doneSomething++;
      else if (rv==-1) {
        DBG_INFO(0, "Error handling card");
      }
      card=LC_Card_List2Iterator_Next(cit);
    } /* while */
    LC_Card_List2Iterator_free(cit);
  }

  return doneSomething?0:1;
}



int ServiceKVK_StoreCardData(LC_CLIENT *cl, LC_CARD *cd) {
  SERVICE_KVK *kvks;
  GWEN_DB_NODE *dbData;
  GWEN_TIME *ti;
  FILE *f;
  GWEN_BUFFER *pbuf;
  GWEN_BUFFER *dbuf;
  const char *s;
  int r;
  char numbuf[32];
  int i;
  GWEN_TYPE_UINT32 pos;

  assert(cl);
  kvks=GWEN_INHERIT_GETDATA(LC_CLIENT, SERVICE_KVK, cl);
  assert(kvks);

  dbData=KVKSCard_GetDbCardData(cd);
  assert(dbData);
  dbData=GWEN_DB_GetGroup(dbData, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                          "kvk/data");
  assert(dbData);

  ti=GWEN_CurrentTime();
  assert(ti);

  pbuf=GWEN_Buffer_new(0, 256, 0, 1);
  s=LC_Service_GetServiceDataDir(cl);
  if (s) {
    GWEN_Buffer_AppendString(pbuf, s);
#ifdef OS_WIN32
    GWEN_Buffer_AppendByte(pbuf, '\\');
#else
    GWEN_Buffer_AppendByte(pbuf, '/');
#endif
  }

  GWEN_Buffer_AppendString(pbuf, "KVK-");

  pos=GWEN_Buffer_GetPos(pbuf);
  i=10;
  while (i) {
#ifdef HAVE_RANDOM
    r=random();
#else
    r=rand();
#endif
    snprintf(numbuf, sizeof(numbuf)-1, "%d", r);
    GWEN_Buffer_AppendString(pbuf, numbuf);
    GWEN_Buffer_AppendString(pbuf, ".dat");
    f=fopen(GWEN_Buffer_GetStart(pbuf), "r");
    if (f)
      fclose(f);
    if (f==0)
      break;
    GWEN_Buffer_Crop(pbuf, 0, pos);
    i--;
  } /* while */

  if (!i) {
    DBG_ERROR(0,
              "WARNING: Could not create a unique name within 10 tries !\n"
              "Please report to bugs@libchipcard.de");
    GWEN_Buffer_free(pbuf);
    GWEN_Time_free(ti);
    return -1;
  }

  pos=GWEN_Buffer_GetPos(pbuf);
  GWEN_Buffer_AppendString(pbuf, ".tmp");
  f=fopen(GWEN_Buffer_GetStart(pbuf), "w+");
  if (f==0) {
    DBG_ERROR(0, "Could not create file \"%s\", reason: %s",
              GWEN_Buffer_GetStart(pbuf),
              strerror(errno));
    GWEN_Buffer_free(pbuf);
    GWEN_Time_free(ti);
    return -1;
  }

  fprintf(f,"Version:libchipcard2-"CHIPCARD_VERSION_FULL_STRING"\n");
  dbuf=GWEN_Buffer_new(0, 32, 0, 1);
  GWEN_Time_toString(ti, "DD.MM.YYYY", dbuf);
  fprintf(f, "Datum:%s\n", GWEN_Buffer_GetStart(dbuf));
  GWEN_Buffer_Reset(dbuf);
  GWEN_Time_toString(ti, "hh:mm:ss", dbuf);
  fprintf(f, "Zeit:%s\n", GWEN_Buffer_GetStart(dbuf));
  GWEN_Time_free(ti);
  GWEN_Buffer_free(dbuf); dbuf=0;

  fprintf(f,"KK-Name:%s\n",
          GWEN_DB_GetCharValue(dbData, "insuranceCompanyName", 0, ""));
  fprintf(f,"KK-Nummer:%s\n",
          GWEN_DB_GetCharValue(dbData, "insuranceCompanyCode", 0, ""));
  fprintf(f,"KVK-Nummer:%s\n",
          GWEN_DB_GetCharValue(dbData, "cardNumber", 0, ""));
  fprintf(f,"V-Nummer:%s\n",
          GWEN_DB_GetCharValue(dbData, "insuranceNumber", 0, ""));
  fprintf(f,"V-Status:%s\n",
          GWEN_DB_GetCharValue(dbData, "insuranceState", 0, ""));
  fprintf(f,"V-Statusergaenzung:%s\n",
          GWEN_DB_GetCharValue(dbData, "eastOrWest", 0, ""));
  fprintf(f,"Titel:%s\n",
          GWEN_DB_GetCharValue(dbData, "title", 0, ""));
  fprintf(f,"Vorname:%s\n",
          GWEN_DB_GetCharValue(dbData, "foreName", 0, ""));
  fprintf(f,"Namenszusatz:%s\n",
          GWEN_DB_GetCharValue(dbData, "nameSuffix", 0, ""));
  fprintf(f,"Familienname:%s\n",
          GWEN_DB_GetCharValue(dbData, "name", 0, ""));
  fprintf(f,"Geburtsdatum:%s\n",
          GWEN_DB_GetCharValue(dbData, "dateOfBirth", 0, ""));
  fprintf(f,"Strasse:%s\n",
          GWEN_DB_GetCharValue(dbData, "addrStreet", 0, ""));
  fprintf(f,"Laendercode:%s\n",
          GWEN_DB_GetCharValue(dbData, "addrState", 0, ""));
  fprintf(f,"PLZ:%s\n",
          GWEN_DB_GetCharValue(dbData, "addrPostalCode", 0, ""));
  fprintf(f,"Ort:%s\n",
          GWEN_DB_GetCharValue(dbData, "addrCity", 0, ""));
  fprintf(f,"gueltig-bis:%s\n",
          GWEN_DB_GetCharValue(dbData, "bestBefore", 0, ""));
  fprintf(f,"Pruefsumme-gueltig:%s\n",
          (KVKSCard_GetCheckSumOk(cd))?"ja":"nein");
  fprintf(f,"Kommentar:derzeit keiner\n");

  if (fclose(f)) {
    DBG_ERROR(0, "Could not close file \"%s\", reason: \n %s",
              GWEN_Buffer_GetStart(pbuf),
              strerror(errno));
    GWEN_Buffer_free(pbuf);
    return -1;
  }

  /* rename */
  dbuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Buffer_AppendBytes(dbuf, GWEN_Buffer_GetStart(pbuf), pos);

  if (rename(GWEN_Buffer_GetStart(pbuf),
             GWEN_Buffer_GetStart(dbuf))) {
    DBG_ERROR(0,
              "Could not rename file \"%s\" to \"%s\", reason: \n %s",
              GWEN_Buffer_GetStart(pbuf),
              GWEN_Buffer_GetStart(dbuf),
              strerror(errno));
    GWEN_Buffer_free(dbuf);
    GWEN_Buffer_free(pbuf);
    return -1;
  }

  GWEN_Buffer_free(dbuf);
  GWEN_Buffer_free(pbuf);
  DBG_NOTICE(0, "Card data written");
  return 0;
}







int main(int argc, char **argv) {
  LC_CLIENT *sv;

  sv=ServiceKVK_new(argc, argv);
  if (!sv) {
    DBG_ERROR(0, "Could not initialize service");
    return 1;
  }

  if (ServiceKVK_Start(sv)) {
    DBG_ERROR(0, "Could not start service");
    LC_Client_free(sv);
    return 1;
  }

  if (LC_Service_Work(sv)) {
    DBG_ERROR(0, "An error occurred");
  }

  DBG_NOTICE(0, "Stopping service \"%s\"", argv[0]);
  GWEN_Socket_Select(0, 0, 0, 1000);

  LC_Client_free(sv);
  return 0;
}







