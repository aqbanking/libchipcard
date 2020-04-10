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
#include <chipcard/cards/zkacard.h>
#include <chipcard/ct/ct_card.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

/*#define ADD_BBBANK_KEYHASH*/

int exportContext(GWEN_DB_NODE *dbArgs, int argc, char **argv)
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
  uint8_t guiid=0;
  short found = 0;


  const GWEN_ARGS args[]= {
          {
                  GWEN_ARGS_FLAGS_HAS_ARGUMENT, /* flags */
                  GWEN_ArgsType_Int,            /* type */
                  "ctxNum",                     /* name */
                  1,                            /* minnum */
                  30,                            /* maxnum */
                  "n",                          /* short option */
                  "ctxNum",                       /* long option */
                  "Specify the context number",      /* short description */
                  "Specify the context number"       /* long description */
          },
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

  contextNumber=GWEN_DB_GetIntValue(db, "ctxNum", 0, 1);
  ctxFileName=GWEN_DB_GetCharValue(db, "ctxFileName", 0, "context.xml");

  res= ZkaCardTool_OpenCard(&lc,&hcard);

  if (res!=LC_Client_ResultOk) {
      showError(hcard, res, "StartWait");
      ZkaCardTool_CloseCard(lc,hcard);
      return RETURNVALUE_WORK;
  }

  res = ZkaCardTool_EnsureAccessPin(hcard, guiid, 0);
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

  dbNotePad=GWEN_DB_Group_new("notepad");
  mbuf=GWEN_Buffer_new(0, 256, 0, 1);
  for ( i = 1; i < ZKACARD_NUM_CONTEXT ; i++) {
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
      GWEN_DB_NODE *dbEntry;

      /* parse record */
      if (GWEN_Buffer_GetBytesLeft(mbuf)) {
        DBG_INFO(LC_LOGDOMAIN, "Parsing entry %d", i);
        dbEntry=GWEN_DB_Group_new("entry");
        GWEN_Buffer_Rewind(mbuf);
        res=LC_Card_ParseRecord(hcard, i, mbuf, dbEntry);
        if (res!=LC_Client_ResultOk) {
          DBG_ERROR(LC_LOGDOMAIN, "Error parsing record %d of EF_NOTEPAD (%d)", i, res);
          GWEN_DB_Group_free(dbEntry);
        }

        /* add new entry */
        DBG_INFO(LC_LOGDOMAIN, "Adding entry %d", i);
        GWEN_DB_AddGroup(dbNotePad, dbEntry);
      }
      else {
        DBG_INFO(LC_LOGDOMAIN, "Entry %d is empty", i);
      }
    }
    GWEN_Buffer_Reset(mbuf);
  }
  GWEN_Buffer_free(mbuf);
  cnt=1;
  dbRecord=GWEN_DB_GetFirstGroup(dbNotePad);
  printf("Notepad\n\n");

  while (dbRecord && cnt< ZKACARD_NUM_CONTEXT) {
    GWEN_DB_NODE *dbCtx;

    dbCtx=GWEN_DB_GetGroup(dbRecord, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "entry/hbciData");
    if (dbCtx) {
      if (cnt == contextNumber)
      {
          found = 1;
          break;
      }
      cnt++;
    }
    dbRecord=GWEN_DB_GetNextGroup(dbRecord);
  }

  if (found)
  {
      GWEN_DB_NODE *dbParams;

      dbParams=GWEN_DB_Group_new("params");

#ifdef ADD_BBBANK_KEYHASH
      {
          int tmp;
          const char *tmpChar;
          const char *tmpBin;
          char val = 0xf0;
          GWEN_DB_SetBinValue(dbRecord, GWEN_DB_FLAGS_OVERWRITE_VARS,"entry/entry",&val,1);
          tmpChar = GWEN_DB_GetCharValue(dbRecord, "entry/hbciData/institute/keyNum", 0, NULL);
          if (tmpChar == NULL)
          {
              GWEN_DB_SetCharValue(dbRecord, GWEN_DB_FLAGS_OVERWRITE_VARS,"entry/hbciData/institute/keyNum","007");
          }

          tmpChar = GWEN_DB_GetCharValue(dbRecord, "entry/hbciData/institute/keyVer", 0, NULL);
          if (tmpChar == NULL)
          {
              GWEN_DB_SetCharValue(dbRecord, GWEN_DB_FLAGS_OVERWRITE_VARS,"entry/hbciData/institute/keyVer","001");
          }

          tmpBin = (char *) GWEN_DB_GetBinValue(dbRecord,"entry/hbciData/institute/keyAlgo", 0, NULL, 0, &tmp);
          if (tmpBin == NULL)
          {
              char val = 0x03;
              GWEN_DB_SetBinValue(dbRecord, GWEN_DB_FLAGS_OVERWRITE_VARS,"entry/hbciData/institute/keyAlgo",&val,1);
          }
          tmpBin = (char *) GWEN_DB_GetBinValue(dbRecord,"entry/hbciData/institute/keyHash", 0, NULL, 0, &tmp);
          if (tmpBin == NULL)
          {
              char hash[32]={
                  0x96, 0xb6, 0xba, 0xa1, 0xad, 0xbf, 0xce, 0xad,
                  0xfb, 0xdb, 0x01, 0x84, 0x80, 0x7a, 0x12, 0x76,
                  0x29, 0xdb, 0x5f, 0xfc, 0x83, 0xb3, 0x4f, 0x98,
                  0x87, 0x50, 0x10, 0x82, 0x40, 0x4d, 0x58, 0x5d
              };
              GWEN_DB_SetBinValue(dbRecord, GWEN_DB_FLAGS_OVERWRITE_VARS,"entry/hbciData/institute/keyHash",(void*) &hash[0],32);
          }

      }
      printf("HBCI Context #%d\n\n", cnt);
      GWEN_DB_Dump(dbRecord, 2);
      printf("\n\n");
#endif
      printf("Writing HBCI Context #%d\n\n", cnt);
      GWEN_DB_Dump(dbRecord, 2);
      printf("\n\n");
      GWEN_DB_WriteFileAs(dbRecord,
                          ctxFileName,
                          "xmldb",
                          dbParams,
                          GWEN_DB_FLAGS_DEFAULT);
  }
  else
  {
      printf("Context #%d not found!\n", contextNumber);
  }

  GWEN_DB_Group_free(dbNotePad);
  /* close card */
  ZkaCardTool_CloseCard(lc,hcard);

  return 0;
}


