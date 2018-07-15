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
#include <gwenhywfar/ct_keyinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int getPublicKey(GWEN_CRYPT_TOKEN *ct,
                 GWEN_DB_NODE *dbArgs,
                 int argc,
                 char **argv)
{

    GWEN_DB_NODE *db;
    int rv;
    int keyNumber;
    int j;
    const char *s;
    uint32_t kid;
    const GWEN_CRYPT_TOKEN_KEYINFO *keyInfo=NULL;


  const GWEN_ARGS args[]={
  {
    GWEN_ARGS_FLAGS_HAS_ARGUMENT, /* flags */
    GWEN_ArgsType_Int,            /* type */
    "keyNum",                     /* name */
    0,                            /* minnum */
    1,                            /* maxnum */
    "n",                          /* short option */
    "keyNum",                       /* long option */
    "Specify the key number",      /* short description */
    "Specify the key number"       /* long description */
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

#if 0
  s=GWEN_DB_GetCharValue(db, "keyNum", 0, 0);
  if (s && 1==sscanf(s, "%d", &j))
      keyNumber=j;
#endif
  keyNumber=GWEN_DB_GetIntValue(db, "keyNum", 0, 1);

  /* get public key info */

  if (keyNumber) {
      const GWEN_CRYPT_TOKEN_CONTEXT *cctx;
      /* get context */
      cctx=GWEN_Crypt_Token_GetContext(ct, 1, 0);
      if (!cctx) {
        DBG_ERROR(GWEN_LOGDOMAIN, "User context not found on crypt token");
        GWEN_Gui_ProgressLog(0,
                 GWEN_LoggerLevel_Error,
                 I18N("User context not found on crypt token"));
        return GWEN_ERROR_NOT_FOUND;
      }

      switch(keyNumber)
      {
      case 2:
          /*AUT*/
          DBG_INFO(GWEN_LOGDOMAIN, "AUT Key");
          kid=GWEN_Crypt_Token_Context_GetSignKeyId(cctx);
          break;
      case 3:
          /*KE*/
          DBG_INFO(GWEN_LOGDOMAIN, "KE Key");
          kid=GWEN_Crypt_Token_Context_GetDecipherKeyId(cctx);
          break;
      case 4:
          /*DS*/
          DBG_INFO(GWEN_LOGDOMAIN, "DS Key");
          kid=GWEN_Crypt_Token_Context_GetAuthSignKeyId(cctx);
          break;
      default:
          DBG_INFO(GWEN_LOGDOMAIN, "Wrong key id");
          return 0;
      }
      keyInfo=GWEN_Crypt_Token_GetKeyInfo(ct, kid,
              GWEN_CRYPT_TOKEN_KEYFLAGS_HASMODULUS |
              GWEN_CRYPT_TOKEN_KEYFLAGS_HASEXPONENT |
              GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYVERSION |
              GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYNUMBER,
              0);
      if (keyInfo==NULL) {
          DBG_ERROR(GWEN_LOGDOMAIN, "Key info not found on zka card");
          GWEN_Gui_ProgressLog(0,
                  GWEN_LoggerLevel_Error,
                  I18N("Key info not found on crypt token"));
          return GWEN_ERROR_NOT_FOUND;
      }
  }
  else {
      DBG_INFO(GWEN_LOGDOMAIN, "No key id");
  }

  if (keyInfo)
  {
      GWEN_Crypt_Token_KeyInfo_Dump(keyInfo);
  }

  return 0;

}

