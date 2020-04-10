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


static
int EnterPinWithPinInfo(LC_CARD *hcard,
                        GWEN_CRYPT_PINTYPE pt,
                        const LC_PININFO *pi,
                        uint32_t guiid)
{
  LC_CLIENT_RESULT res;
  int maxErrors;
  int currentErrors;

  assert(hcard);
  assert(pi);



  res=LC_Card_GetPinStatus(hcard,
                           LC_PinInfo_GetId(pi),
                           &maxErrors,
                           &currentErrors);
  if (res!=LC_Client_ResultNotSupported) {
    if (res!=LC_Client_ResultOk) {
      DBG_ERROR(LC_LOGDOMAIN,
                "Unable to read status of pin %x (%d)",
                LC_PinInfo_GetId(pi),
                res);
      return LC_Crypt_Token_ResultToError(res);
    }

    if ((currentErrors==maxErrors)) {
        printf("PIN PTC ok. Nothing to be done.\n");


      DBG_ERROR(LC_LOGDOMAIN,
                "Maximum pin try counter. Nothing to be done");
      return 0;
    }
  }
  else {
    DBG_INFO(LC_LOGDOMAIN,
             "Unable to read pin status for pin %02x (not supported)",
             LC_PinInfo_GetId(pi));
  }

  if ((pt!=GWEN_Crypt_PinType_Manage) &&
      (LC_Card_GetReaderFlags(hcard) & LC_READER_FLAGS_KEYPAD)) {
    uint32_t bid;
    int triesLeft=-1;

    DBG_INFO(LC_LOGDOMAIN, "Terminal has a keypad, will ask for pin.");
    printf("Bad pin entered at least once before.\n ");
    printf("Maximum number of tries:   %d\n",maxErrors);
    printf("Remaining number of tries: %d\n",currentErrors);
    printf("Enter pin to reset try counter...");

    res=LC_Card_IsoPerformVerification(hcard, 0, pi, &triesLeft);

    if (res!=LC_Client_ResultOk) {
      /* tell the user about end of pin verification */

      DBG_ERROR(LC_LOGDOMAIN, "sw1=%02x sw2=%02x (%s)",
                LC_Card_GetLastSW1(hcard),
                LC_Card_GetLastSW2(hcard),
                LC_Card_GetLastText(hcard));
      if (LC_Card_GetLastSW1(hcard)==0x90 &&
                     LC_Card_GetLastSW2(hcard)==0x00)
      {
          res=LC_Card_GetPinStatus(hcard,
                                   LC_PinInfo_GetId(pi),
                                   &maxErrors,
                                   &currentErrors);
          printf("PIN verification succesfull\n");
          printf("Maximum number of tries:   %d\n",maxErrors);
          printf("Remaining number of tries: %d\n",currentErrors);
          return 0;
      }
      else if (LC_Card_GetLastSW1(hcard)==0x63) {
        switch (LC_Card_GetLastSW2(hcard)) {
        case 0xc0: /* no error left */
            printf("PIN verification failed. PIN is disabled.\n");
            return GWEN_ERROR_BAD_PIN_0_LEFT;
        case 0xc1: /* one left */
            printf("PIN verification failed. 1 try left.\n");
          return GWEN_ERROR_BAD_PIN_1_LEFT;
        case 0xc2: /* two left */
            printf("PIN verification failed. 2 tries left.\n");
          return GWEN_ERROR_BAD_PIN_2_LEFT;
        default:   /* unknown error */
            printf("Unkown error...\n");
          return GWEN_ERROR_BAD_PIN;
        } // switch
      }
      else if (LC_Card_GetLastSW1(hcard)==0x69 &&
               LC_Card_GetLastSW2(hcard)==0x83) {
        DBG_ERROR(LC_LOGDOMAIN, "Card unusable");
        printf("PIN is disabled.\n");
        return GWEN_ERROR_IO;
      }
      else if (LC_Card_GetLastSW1(hcard)==0x64 &&
               LC_Card_GetLastSW2(hcard)==0x01) {
        DBG_ERROR(LC_LOGDOMAIN, "Aborted by user");
        return GWEN_ERROR_USER_ABORTED;
      }
      else {
        if (triesLeft>=0) {
          switch (triesLeft) {
          case 0: /* no error left */
              printf("PIN verification failed. PIN is disabled.\n");
            return GWEN_ERROR_BAD_PIN_0_LEFT;
          case 1: /* one left */
              printf("PIN verification failed. 1 try left.\n");
            return GWEN_ERROR_BAD_PIN_1_LEFT;
          case 2: /* two left */
              printf("PIN verification failed. 2 tries left.\n");
            return GWEN_ERROR_BAD_PIN_2_LEFT;
          default:   /* unknown count */
              printf("Unkown error...\n");
            return GWEN_ERROR_BAD_PIN;
          } // switch
        }

        return GWEN_ERROR_IO;
      }
    } /* if not ok */
    else {
      /* PIN ok */
      DBG_INFO(LC_LOGDOMAIN, "Pin ok");
      res=LC_Card_GetPinStatus(hcard,
                               LC_PinInfo_GetId(pi),
                               &maxErrors,
                               &currentErrors);
      printf("PIN verification succesfull\n");
      printf("Maximum number of tries:   %d\n",maxErrors);
      printf("Remaining number of tries: %d\n",currentErrors);
      return 0;
    }
  } /* if hasKeyPad */
  else {
#if 1
    DBG_ERROR(LC_LOGDOMAIN, "Card Reader without key pad!\nCould not verify pin!\n");
    return GWEN_ERROR_IO;
#else
    unsigned char pinBuffer[64];
    int mres;
    int pinMaxLen;
    unsigned int pinLength;
    unsigned int origPinLength;
    uint32_t pflags=0;
    GWEN_CRYPT_PINENCODING pe;
    int triesLeft=-1;

    DBG_INFO(LC_LOGDOMAIN, "No keypad (or disabled), will ask for PIN");
    memset(pinBuffer, 0, sizeof(pinBuffer));

    pe=LC_PinInfo_GetEncoding(pi);
    if (pt==GWEN_Crypt_PinType_Manage)
      pflags|=GWEN_GUI_INPUT_FLAGS_ALLOW_DEFAULT;
    pflags|=GWEN_GUI_INPUT_FLAGS_NUMERIC;
    pinLength=0;
    pinMaxLen=LC_PinInfo_GetMaxLength(pi);
    if (!pinMaxLen || pinMaxLen>sizeof(pinBuffer)-1)
      pinMaxLen=sizeof(pinBuffer)-1;
    mres=LC_Crypt_Token__GetPin(ct,
                                hcard,
                                LC_PinInfo_GetId(pi),
                                pt,
                                pe,
                                pflags,
                                pinBuffer,
                                LC_PinInfo_GetMinLength(pi),
                                pinMaxLen,
                                &pinLength,
                                guiid);
    if (mres!=0) {
      DBG_ERROR(LC_LOGDOMAIN, "Error asking for PIN, aborting");
      memset(pinBuffer, 0, sizeof(pinBuffer));
      return mres;
    }
    origPinLength=pinLength;

    if (pinLength<pinMaxLen && LC_PinInfo_GetFiller(pi)) {
      int i;
      unsigned char c;

      c=(unsigned char)LC_PinInfo_GetFiller(pi);
      for (i=pinLength; i<pinMaxLen; i++)
        pinBuffer[i]=c;
      pinLength=pinMaxLen;
    }

    DBG_INFO(LC_LOGDOMAIN, "Verifying the PIN");
    res=LC_Card_IsoVerifyPin(hcard,
                             0,
                             pi,
                             pinBuffer,
                             pinLength,
                             &triesLeft);
    if (res!=LC_Client_ResultOk) {
      DBG_ERROR(LC_LOGDOMAIN, "sw1=%02x sw2=%02x (%s)",
                LC_Card_GetLastSW1(hcard),
                LC_Card_GetLastSW2(hcard),
                LC_Card_GetLastText(hcard));

      if (LC_Card_GetLastSW1(hcard)==0x63) {
        /* set pin status */
        GWEN_Crypt_Token_SetPinStatus(ct,
                                      pt,
                                      pe,
                                      pflags,
                                      pinBuffer,
                                      origPinLength,
                                      0,
                                      guiid);

        switch (LC_Card_GetLastSW2(hcard)) {
        case 0xc0: /* no error left */
          return GWEN_ERROR_BAD_PIN_0_LEFT;
        case 0xc1: /* one left */
          return GWEN_ERROR_BAD_PIN_1_LEFT;
        case 0xc2: /* two left */
          return GWEN_ERROR_BAD_PIN_2_LEFT;
        default:
          return GWEN_ERROR_BAD_PIN;
        } // switch
      }
      else if (LC_Card_GetLastSW1(hcard)==0x69 &&
               LC_Card_GetLastSW2(hcard)==0x83) {
        /* set pin status */
        GWEN_Crypt_Token_SetPinStatus(ct,
                                      pt,
                                      pe,
                                      pflags,
                                      pinBuffer,
                                      origPinLength,
                                      0,
                                      guiid);
        DBG_ERROR(LC_LOGDOMAIN, "Card unusable");
        return GWEN_ERROR_IO;
      }
      else if (LC_Card_GetLastSW1(hcard)==0x64 &&
               LC_Card_GetLastSW2(hcard)==0x01) {
        return GWEN_ERROR_USER_ABORTED;
      }
      else {
        if (triesLeft>=0) {
          /* set pin status */
          GWEN_Crypt_Token_SetPinStatus(ct,
                                        pt,
                                        pe,
                                        pflags,
                                        pinBuffer,
                                        origPinLength,
                                        0,
                                        guiid);
          switch (triesLeft) {
          case 0: /* no error left */
            return GWEN_ERROR_BAD_PIN_0_LEFT;
          case 1: /* one left */
            return GWEN_ERROR_BAD_PIN_1_LEFT;
          case 2: /* two left */
            return GWEN_ERROR_BAD_PIN_2_LEFT;
          default:   /* unknown count */
            return GWEN_ERROR_BAD_PIN;
          } // switch
        }
        DBG_ERROR(LC_LOGDOMAIN, "Unknown error");
        return GWEN_ERROR_IO;
      }
    } // if not ok
    else {
      DBG_INFO(LC_LOGDOMAIN, "PIN ok");
      /* set pin status */
      GWEN_Crypt_Token_SetPinStatus(ct,
                                    pt,
                                    pe,
                                    pflags,
                                    pinBuffer,
                                    origPinLength,
                                    1,
                                    guiid);
    }
#endif
  } // if no keyPad

  return 0;
}


int resetPtc(GWEN_DB_NODE *dbArgs, int argc, char **argv)
{

  GWEN_DB_NODE *db=NULL;
  GWEN_DB_NODE *dbNotePad;
  GWEN_DB_NODE *dbRecord;
  int rv;
  int keyNumber;
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

  const GWEN_ARGS args[]= {
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


  res= ZkaCardTool_OpenCard(&lc,&hcard);

  if (res!=LC_Client_ResultOk) {
      showError(hcard, res, "StartWait");
      ZkaCardTool_CloseCard(lc,hcard);
      return RETURNVALUE_WORK;
  }

  ZkaCardTool_EnsureAccessPin(hcard, guuid,1);

  ZkaCardTool_CloseCard(lc,hcard);

  return 0;
}


