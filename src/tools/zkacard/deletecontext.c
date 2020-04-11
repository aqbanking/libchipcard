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





int deleteContext(GWEN_DB_NODE *dbArgs, int argc, char **argv)
{

  GWEN_DB_NODE *db=NULL;
  GWEN_DB_NODE *dbNotePad;
  GWEN_DB_NODE *dbRecord;
  int rv;
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
  short ctxNum = 0;
  short recordCnt = 0;


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

  ctxNum=GWEN_DB_GetIntValue(db, "ctxNum", 0, 0);



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


  dbNotePad=GWEN_DB_Group_new("notepad");
  mbuf=GWEN_Buffer_new(0, 256, 0, 1);

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
  recordCnt=1;
  dbRecord=GWEN_DB_GetFirstGroup(dbNotePad);
  printf("Notepad\n\n");

  while (dbRecord && recordCnt< ZKACARD_NUM_CONTEXT) {
    GWEN_DB_NODE *dbCtx;

    dbCtx=GWEN_DB_GetGroup(dbRecord, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "entry/hbciData");
    if (dbCtx) {
      printf("HBCI Context #%d\n\n", cnt);
      GWEN_DB_Dump(dbCtx, 2);
      printf("\n\n");
      if (cnt == ctxNum)
      {
          found = 1;
          break;
      }
      cnt++;
    }
    else {
      printf("Other Application Data\n");
      GWEN_DB_Dump(dbRecord, 2);
      printf("\n\n");
    }
    dbRecord=GWEN_DB_GetNextGroup(dbRecord);
    recordCnt++;
  }



  if (found)
  {
      /* create record from DB_NODE */
      /* create record data */
      GWEN_DB_NODE *dbCtx = GWEN_DB_GetGroup(dbRecord, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "entry");
      mbuf=GWEN_Buffer_new(0, 2, 0, 1);
      GWEN_Buffer_AppendByte(mbuf,0x0);
      GWEN_Buffer_AppendByte(mbuf,0x0);
      GWEN_Buffer_Rewind(mbuf);

      /* write record */
      res=LC_Card_IsoUpdateRecord(hcard, LC_CARD_ISO_FLAGS_RECSEL_GIVEN,
                                  recordCnt,
                                  GWEN_Buffer_GetStart(mbuf),
                                  GWEN_Buffer_GetUsedBytes(mbuf));
      GWEN_Buffer_free(mbuf);
      if (res!=LC_Client_ResultOk) {
        DBG_INFO(LC_LOGDOMAIN, "Error writing new context.");
        return res;
      }
      else
      {
          DBG_INFO(LC_LOGDOMAIN, "Deleted context %d!\n", cnt);
          printf("Deleted context %d!\n", cnt);
      }

  }
  else
  {
      DBG_INFO(LC_LOGDOMAIN, "Context %d could not be deleted!",ctxNum);
      printf("Context %d could not be deleted!\n",ctxNum);
  }

  GWEN_DB_Group_free(dbNotePad);
  /* close card */
  ZkaCardTool_CloseCard(lc,hcard);

  return 0;
}


