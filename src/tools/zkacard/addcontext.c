/***************************************************************************
 $RCSfile$
 -------------------
 cvs         : $Id$
 begin       : Sat Jun 23 2018
 copyright   : (C) 2018 by Martin Preuss, Stefan Bayer
 email       : martin@libchipcard.de, stefan.bayer@stefanbayer.net


 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "global.h"

#include <gwenhywfar/text.h>
#include <gwenhywfar/args.h>
#include <gwenhywfar/gui.h>
#include <chipcard/client.h>
#include <gwenhywfar/msgengine.h>
#include <chipcard/cards/zkacard.h>
#include <chipcard/ct/ct_card.h>
#include <chipcard/card.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>



static int ZkaCardTool__CreateContextRecord(GWEN_BUFFER *buf,
                                            GWEN_DB_NODE *dbRecord)
{
  /* we have to do this manually, does not work with CreateRecord due to ambiguities in the definition */
  GWEN_BUFFER *localBuffer;
  int         totalLen=0;
  int         version = 0;
  localBuffer = GWEN_Buffer_new(NULL,236,0,0);

  /* version */
  {
      int   versionLen,j;
      const char  *versionStr = GWEN_DB_GetCharValue(dbRecord, "hbciData/version", 0, NULL);
      versionLen = strlen(versionStr);
      assert(versionLen == 3);
      GWEN_Buffer_AppendByte(localBuffer,0xc0);
      GWEN_Buffer_AppendByte(localBuffer,0x03);
      GWEN_Buffer_AppendString(localBuffer,versionStr);
      totalLen += 5;
      if (versionStr && 1==sscanf(versionStr, "%d", &j))
        version=j;
  }

  /* HBCI-Institutsparameterblock */
  {
      const char * bankName;
      int          bankNameLen;
      const char * country;
      const char * bankCode;
      int          bankCodeLen;
      const char * keyNum;
      const char * keyVer;
      const char * hashAlgo;
      int          hashAlgoLen;
      const char * keyHash;
      int          keyHashLen;
      int          keyStatus;
      int          instSize = 0;

      GWEN_BUFFER *instBuffer = GWEN_Buffer_new(NULL,96,0,0);


      bankName = GWEN_DB_GetCharValue(dbRecord, "hbciData/institute/bankName", 0, NULL);
      if ( bankName )
      {
          bankNameLen = strlen(bankName);
          GWEN_Buffer_AppendByte(instBuffer,0xc1);
          GWEN_Buffer_AppendByte(instBuffer,bankNameLen);
          GWEN_Buffer_AppendString(instBuffer,bankName);
          instSize += (2+bankNameLen);
      }

      country = GWEN_DB_GetCharValue(dbRecord, "hbciData/institute/country", 0, NULL);
      if ( country )
      {
          assert(strlen(country) == 3);
          GWEN_Buffer_AppendByte(instBuffer,0xc2);
          GWEN_Buffer_AppendByte(instBuffer,0x03);
          GWEN_Buffer_AppendString(instBuffer,country);
          instSize += 5;
      }
      else
      {
          DBG_ERROR(LC_LOGDOMAIN,"No country code in context!\n");
          GWEN_Buffer_free(instBuffer);
          GWEN_Buffer_free(localBuffer);
          return GWEN_ERROR_ABORTED;
      }

      bankCode = GWEN_DB_GetCharValue(dbRecord, "hbciData/institute/bankCode", 0, NULL);
      if ( bankCode )
      {
          bankCodeLen = strlen(bankCode);
          GWEN_Buffer_AppendByte(instBuffer,0xc3);
          GWEN_Buffer_AppendByte(instBuffer,bankCodeLen);
          GWEN_Buffer_AppendString(instBuffer,bankCode);
          instSize += (2+bankCodeLen);
      }
      else
      {
          DBG_ERROR(LC_LOGDOMAIN,"No bank code in context!\n");
          GWEN_Buffer_free(instBuffer);
          GWEN_Buffer_free(localBuffer);
          return GWEN_ERROR_ABORTED;
      }

      keyNum = GWEN_DB_GetCharValue(dbRecord, "hbciData/institute/keyNum", 0, NULL);
      keyVer = GWEN_DB_GetCharValue(dbRecord, "hbciData/institute/keyVer", 0, NULL);
      hashAlgo = GWEN_DB_GetBinValue(dbRecord, "hbciData/institute/keyAlgo", 0, NULL, 0, &hashAlgoLen);
      keyHash = GWEN_DB_GetBinValue(dbRecord, "hbciData/institute/keyHash", 0, NULL, 0, &keyHashLen);

      if ( keyHash && keyNum && keyVer && hashAlgo)
      {
          assert(strlen(keyNum) == 3);
          assert(strlen(keyVer) == 3);
          assert(hashAlgoLen == 1);
          assert(keyHashLen == 32 || keyHashLen == 20);
          GWEN_Buffer_AppendByte(instBuffer,0xc4);
          GWEN_Buffer_AppendByte(instBuffer,0x27);
          GWEN_Buffer_AppendString(instBuffer,keyNum);
          GWEN_Buffer_AppendString(instBuffer,keyVer);
          GWEN_Buffer_AppendByte(instBuffer,*hashAlgo);
          GWEN_Buffer_AppendBytes(instBuffer,keyHash,keyHashLen);
          if ( version == 2 && keyHashLen == 20 )
          {
              int i;
              char tmp = 0x0;
              for (i = 0 ; i < 12 ; i++ ) GWEN_Buffer_AppendByte(instBuffer,tmp);
              keyHashLen = 32;
          }
          instSize += 9 + keyHashLen;
      }

      keyStatus = GWEN_DB_GetIntValue(dbRecord, "hbciData/institute/keyStatus", 0, 1);
      GWEN_Buffer_AppendByte(instBuffer,0xc5);
      GWEN_Buffer_AppendByte(instBuffer,0x01);
      GWEN_Buffer_AppendByte(instBuffer,keyStatus);
      instSize += 3;

      GWEN_Buffer_AppendByte(localBuffer,0xe1);
      GWEN_Buffer_AppendByte(localBuffer,instSize);

      GWEN_Buffer_Rewind(instBuffer);
      assert(GWEN_Buffer_GetUsedBytes(instBuffer) == instSize);
      GWEN_Buffer_AppendBytes(localBuffer,GWEN_Buffer_GetStart(instBuffer),instSize);
      totalLen += 2+instSize;

      GWEN_Buffer_free(instBuffer);
  }

  /* comm Data */
  {
      int          commType;
      const char  *address;
      int          addrLen;

      commType = GWEN_DB_GetIntValue(dbRecord, "hbciData/commData/commType", 0, 0);
      assert(commType == 2);
      address = GWEN_DB_GetCharValue(dbRecord, "hbciData/commData/address", 0, NULL);
      if ( address )
      {
          addrLen = strlen(address);
          assert(addrLen < 51);
      }
      else
      {
          DBG_ERROR(LC_LOGDOMAIN,"No comm address in context!\n");
          GWEN_Buffer_free(localBuffer);
          return GWEN_ERROR_ABORTED;
      }
      GWEN_Buffer_AppendByte(localBuffer,0xe2);
      GWEN_Buffer_AppendByte(localBuffer,5+addrLen);
      GWEN_Buffer_AppendByte(localBuffer,0xc6);
      GWEN_Buffer_AppendByte(localBuffer,0x01);
      GWEN_Buffer_AppendByte(localBuffer,commType);
      GWEN_Buffer_AppendByte(localBuffer,0xc7);
      GWEN_Buffer_AppendByte(localBuffer,addrLen);
      GWEN_Buffer_AppendString(localBuffer,address);
      totalLen += ( 7 + addrLen );
  }

  {
      /* customer parameters */
      const char * userId;
      int          userIdLen = 0;
      const char * customerId;
      int          customerIdLen = 0;
      const char * signKeyNum;
      const char * signKeyVer;
      const char * cryptKeyNum;
      const char * cryptKeyVer;
      const char * authKeyNum;
      const char * authKeyVer;
      int          userLen = 0;
      int          keyNumLen = 0;

      userId = GWEN_DB_GetCharValue(dbRecord,"hbciData/user/userId",0,NULL);
      customerId = GWEN_DB_GetCharValue(dbRecord,"hbciData/user/customerId",0,NULL);

      signKeyNum = GWEN_DB_GetCharValue(dbRecord,"hbciData/user/signKeyNum",0,NULL);
      signKeyVer = GWEN_DB_GetCharValue(dbRecord,"hbciData/user/signKeyVer",0,NULL);

      cryptKeyNum = GWEN_DB_GetCharValue(dbRecord,"hbciData/user/cryptKeyNum",0,NULL);
      cryptKeyVer = GWEN_DB_GetCharValue(dbRecord,"hbciData/user/cryptKeyVer",0,NULL);

      authKeyNum = GWEN_DB_GetCharValue(dbRecord,"hbciData/user/authKeyNum",0,NULL);
      authKeyVer = GWEN_DB_GetCharValue(dbRecord,"hbciData/user/authKeyVer",0,NULL);

      if ( userId )
      {
          assert(signKeyNum && signKeyVer && cryptKeyNum && cryptKeyVer);
          userIdLen = strlen(userId);
          if (customerId)
          {
              customerIdLen = strlen(customerId) + 2;
          }
          userLen = userIdLen + 2 +customerIdLen + 14;
          if  (authKeyNum && authKeyVer)
          {
              userLen += 6;
          }

          GWEN_Buffer_AppendByte(localBuffer,0xe3);
          GWEN_Buffer_AppendByte(localBuffer,userLen);
          GWEN_Buffer_AppendByte(localBuffer,0xc8);
          GWEN_Buffer_AppendByte(localBuffer,userIdLen);
          GWEN_Buffer_AppendString(localBuffer,userId);

          if ( customerId )
          {
              GWEN_Buffer_AppendByte(localBuffer,0xc9);
              GWEN_Buffer_AppendByte(localBuffer,customerIdLen-2);
              GWEN_Buffer_AppendString(localBuffer,customerId);
          }

          GWEN_Buffer_AppendByte(localBuffer,0xca);
          GWEN_Buffer_AppendByte(localBuffer,(authKeyNum && authKeyVer)?0x12:0x0c);
          GWEN_Buffer_AppendString(localBuffer,signKeyNum);
          GWEN_Buffer_AppendString(localBuffer,signKeyVer);
          GWEN_Buffer_AppendString(localBuffer,cryptKeyNum);
          GWEN_Buffer_AppendString(localBuffer,cryptKeyVer);
          if  (authKeyNum && authKeyVer)
          {
              GWEN_Buffer_AppendString(localBuffer,authKeyNum);
              GWEN_Buffer_AppendString(localBuffer,authKeyVer);
          }

      }
      totalLen += ( userLen + 2);


  }


  /* finalize everything */

  GWEN_Buffer_Rewind(localBuffer);
  assert(totalLen == GWEN_Buffer_GetUsedBytes(localBuffer));

  GWEN_Buffer_AppendByte(buf,0xf0);
  assert(totalLen < 239);
  if (totalLen > 127)
  {
      GWEN_Buffer_AppendByte(buf,0x81);
  }
  GWEN_Buffer_AppendByte(buf,totalLen);
  GWEN_Buffer_AppendBytes(buf,GWEN_Buffer_GetStart(localBuffer),totalLen);

  GWEN_Buffer_free(localBuffer);

  return LC_Client_ResultOk;
}


int addContext(GWEN_DB_NODE *dbArgs, int argc, char **argv)
{

  GWEN_DB_NODE *db=NULL;
  GWEN_DB_NODE *dbNotePad;
  GWEN_DB_NODE *dbRecord;
  int rv;
  int contextNumber;
  const char *ctxFileName;
  int j;
  const char *s;
  int i;
  LC_CLIENT_RESULT res;
  GWEN_BUFFER *mbuf;
  const GWEN_CRYPT_TOKEN_CONTEXT *cctx;
  uint8_t cnt;
  LC_CARD *hcard=0;
  LC_CLIENT *lc;
  uint8_t v=0;
  uint8_t haveAccessPin=0;
  const LC_PININFO *pi;
  uint8_t guuid=0;
  short found = 0;


  const GWEN_ARGS args[]= {
          {
            GWEN_ARGS_FLAGS_HAS_ARGUMENT, /* flags */
            GWEN_ArgsType_Char,            /* type */
            "ctxFileName",                     /* name */
            1,                            /* minnum */
            1,                            /* maxnum */
            "f",                          /* short option */
            "ctxFileName",                       /* long option */
            "Specify the context file name",      /* short description */
            "Specify the context file name"       /* long description */
          },
    {
      GWEN_ARGS_FLAGS_HELP | GWEN_ARGS_FLAGS_LAST, /* flags */
      GWEN_ArgsType_Int,            /* type */
      "help",                       /* name */
      0,                            /* minnum */
      0,                            /* maxnum */
      "h",                          /* short option */
      "help",                       /* long option */
      "Show this help screen",      /* short description */
      "Show this help screen"       /* long description */
    }

  };

  db=GWEN_DB_GetGroup(dbArgs, GWEN_DB_FLAGS_DEFAULT, "local");
  rv=GWEN_Args_Check(argc, argv, 1,
                     0 /*GWEN_ARGS_MODE_ALLOW_FREEPARAM*/,
                     args,
                     db);

  if (rv==GWEN_ARGS_RESULT_ERROR) {
    fprintf(stderr, "ERROR: Could not parse arguments\n");
    return 1;
  }
  else if (rv==GWEN_ARGS_RESULT_HELP) {
    GWEN_BUFFER *ubuf;

    ubuf=GWEN_Buffer_new(0, 1024, 0, 1);
    if (GWEN_Args_Usage(args, ubuf, GWEN_ArgsOutType_Txt)) {
      fprintf(stderr, "ERROR: Could not create help string\n");
      return 1;
    }
    fprintf(stderr, "%s\n", GWEN_Buffer_GetStart(ubuf));
    GWEN_Buffer_free(ubuf);
    return 0;
  }

  ctxFileName=GWEN_DB_GetCharValue(db, "ctxFileName", 0, "context.xml");

  res= ZkaCardTool_OpenCard(&lc,&hcard);

  if (res!=LC_Client_ResultOk) {
      showError(hcard, res, "StartWait");
      ZkaCardTool_CloseCard(lc,hcard);
      return RETURNVALUE_WORK;
  }

  res = ZkaCardTool_EnsureAccessPin(hcard, guuid, 0);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error in PIN verification (%d)", res);
    ZkaCardTool_CloseCard(lc,hcard);
    return GWEN_ERROR_NOT_OPEN;
  }


  res=LC_Card_SelectMf(hcard);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error selecting MF (%d)", res);
    ZkaCardTool_CloseCard(lc,hcard);
    return GWEN_ERROR_NOT_OPEN;
  }

  res=LC_Card_SelectDf(hcard, "DF_NOTEPAD");
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error selecting DF_NOTEPAD (%d)", res);
    ZkaCardTool_CloseCard(lc,hcard);
    return GWEN_ERROR_NOT_OPEN;
  }

  res=LC_Card_SelectEf(hcard, "EF_NOTEPAD");
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error selecting EF_NOTEPAD (%d)", res);
    ZkaCardTool_CloseCard(lc,hcard);
    return GWEN_ERROR_NOT_OPEN;
  }

  {
      GWEN_DB_NODE *dbParams;
      char val = 0xf0;

      dbRecord=GWEN_DB_Group_new("entry");
      /*GWEN_DB_SetBinValue(dbRecord, GWEN_DB_FLAGS_OVERWRITE_VARS,"entry",&val,1);*/
      dbParams=GWEN_DB_Group_new("params");
      GWEN_DB_ReadFileAs(dbRecord,
              ctxFileName,
              "xmldb",
              dbParams,
              GWEN_DB_FLAGS_DEFAULT);
  }

  dbNotePad=GWEN_DB_Group_new("notepad");
  mbuf=GWEN_Buffer_new(0, 256, 0, 1);
  /* search for first empty record */
  for (i=1; i<ZKACARD_NUM_CONTEXT; i++) {
    /* read record */
    DBG_INFO(LC_LOGDOMAIN, "Reading entry %d", i);
    res=LC_Card_IsoReadRecord(hcard, LC_CARD_ISO_FLAGS_RECSEL_GIVEN, i, mbuf);

    GWEN_Buffer_Rewind(mbuf);
    if (res!=LC_Client_ResultOk) {
      if (LC_Card_GetLastSW1(hcard)==0x6a &&
          LC_Card_GetLastSW2(hcard)==0x83) {
        DBG_INFO(LC_LOGDOMAIN, "All records read (%d)", i-1);
        break;
      }
      else {
        DBG_ERROR(LC_LOGDOMAIN, "Error reading record %d of EF_NOTEPAD (%d)", i, res);
      }
    }
    else {

      if (GWEN_Buffer_GetBytesLeft(mbuf)) {
        char firstByte;
        DBG_INFO(LC_LOGDOMAIN, "Check if record %d of EF_NOTEPAD is empty", i);
        GWEN_Buffer_Rewind(mbuf);
        firstByte = (char) GWEN_Buffer_ReadByte(mbuf);
        if ( firstByte == 0x0 )
        {
            DBG_INFO(LC_LOGDOMAIN, "Record %d of EF_NOTEPAD is empty, use as record for new context", i);
            found = 1;
            break;
        }


      }

    }
    GWEN_Buffer_Reset(mbuf);
  }
  GWEN_Buffer_free(mbuf);


  if (found)
  {
      /* create record from DB_NODE */
      /* create record data */
      GWEN_DB_NODE *dbCtx = GWEN_DB_GetGroup(dbRecord, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "entry");
      mbuf=GWEN_Buffer_new(0, 239, 0, 1);
      if (ZkaCardTool__CreateContextRecord(mbuf,dbCtx)) {
        DBG_ERROR(LC_LOGDOMAIN, "Error creating record %d for new context", i);
        GWEN_Buffer_free(mbuf);
        ZkaCardTool_CloseCard(lc,hcard);
        return LC_Client_ResultDataError;
      }
      GWEN_Buffer_Rewind(mbuf);

      /* write record */
      res=LC_Card_IsoUpdateRecord(hcard, LC_CARD_ISO_FLAGS_RECSEL_GIVEN, i,
                                  GWEN_Buffer_GetStart(mbuf),
                                  GWEN_Buffer_GetUsedBytes(mbuf));
      GWEN_Buffer_free(mbuf);
      if (res!=LC_Client_ResultOk) {
        DBG_INFO(LC_LOGDOMAIN, "Error writing new context.");
        ZkaCardTool_CloseCard(lc,hcard);
        return res;
      }

  }
  else
  {
      DBG_INFO(LC_LOGDOMAIN, "New context could not be added, EF_NOTEPAD is full!");
      printf("New context could not be added, EF_NOTEPAD is full!\n");
  }

  GWEN_DB_Group_free(dbNotePad);
  ZkaCardTool_CloseCard(lc,hcard);

  return 0;
}


