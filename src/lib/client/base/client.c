/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: card.c 187 2006-06-15 16:13:23Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define LC_CARD_EXTEND_CLIENT

#include "client_p.h"
#include "card_l.h"
#include "mon/monitor_l.h"
#include <chipcard/sharedstuff/msgengine.h>

#include <gwenhywfar/gwenhywfar.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/directory.h>
#include <gwenhywfar/xml.h>
#include <gwenhywfar/stringlist.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#ifdef OS_WIN32
# define DIRSEP "\\"
#else
# define DIRSEP "/"
#endif


static int lc_client__initcounter=0;
static GWEN_XMLNODE *lc_client__card_nodes=0;
static GWEN_XMLNODE *lc_client__app_nodes=0;
static GWEN_DB_NODE *lc_client__config=0;


GWEN_INHERIT_FUNCTIONS(LC_CLIENT)



GWEN_DB_NODE *LC_Client_GetCommonConfig() {
  return lc_client__config;
}



int LC_Client_InitCommon() {
  if (lc_client__initcounter==0) {
    GWEN_ERRORCODE err;
    GWEN_XMLNODE *n;
    GWEN_DB_NODE *db;
    GWEN_BUFFER *fbuf;
    FILE *f;

    err=GWEN_Init();
    if (!GWEN_Error_IsOk(err)) {
      DBG_ERROR_ERR(LC_LOGDOMAIN, err);
      return -1;
    }

    if (!GWEN_Logger_Exists(LC_LOGDOMAIN)) {
      const char *s;

      /* only set our logger if it not already has been */
      GWEN_Logger_Open(LC_LOGDOMAIN, "chipcard3-client", 0,
                       GWEN_LoggerTypeConsole,
                       GWEN_LoggerFacilityUser);
      GWEN_Logger_SetLevel(LC_LOGDOMAIN, GWEN_LoggerLevelWarning);

      s=getenv("LC_LOGLEVEL");
      if (s) {
        GWEN_LOGGER_LEVEL ll;

        ll=GWEN_Logger_Name2Level(s);
        if (ll!=GWEN_LoggerLevelUnknown) {
          GWEN_Logger_SetLevel(LC_LOGDOMAIN, ll);
          DBG_WARN(0,
                   "Overriding loglevel for Lichipcard-Client with \"%s\"",
                   s);
        }
        else {
          DBG_ERROR(0, "Unknown loglevel \"%s\"",
                    s);
        }
      }
      else {
        GWEN_Logger_SetLevel(LC_LOGDOMAIN, GWEN_LoggerLevelWarning);
      }
    }

    /* load configuration file */
    db=GWEN_DB_Group_new("config");
    fbuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_AppendString(fbuf, LC_CLIENT_CONFIG_DIR DIRSEP);
    GWEN_Buffer_AppendString(fbuf, LC_CLIENT_CONFIG_FILE);
    DBG_INFO(LC_LOGDOMAIN,
             "Trying config file \"%s\"",
             GWEN_Buffer_GetStart(fbuf));
    f=fopen(GWEN_Buffer_GetStart(fbuf), "r");
    if (f==0) {
      GWEN_Buffer_AppendString(fbuf, ".default");
      DBG_INFO(LC_LOGDOMAIN,
               "Trying config file \"%s\"",
               GWEN_Buffer_GetStart(fbuf));
      f=fopen(GWEN_Buffer_GetStart(fbuf), "r");
    }
    if (f) {
      fclose(f);
      if (GWEN_DB_ReadFile(db, GWEN_Buffer_GetStart(fbuf),
			   GWEN_DB_FLAGS_DEFAULT |
			   GWEN_PATH_FLAGS_CREATE_GROUP)) {
	DBG_ERROR(LC_LOGDOMAIN,
		  "Error in configuration file \"%s\"",
		  GWEN_Buffer_GetStart(fbuf));
	GWEN_Buffer_free(fbuf);
	return -1;
      }
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN,
		"Client configuration file not found, "
		"using defaults");
    }
    GWEN_Buffer_free(fbuf);
    lc_client__config=db;

    /* load card XML files */
    n=GWEN_XMLNode_new(GWEN_XMLNodeTypeTag, "cards");
    if (LC_Client_ReadXmlFiles(n, "cards", "card")) {
      DBG_ERROR(LC_LOGDOMAIN, "Could not read card files");
      GWEN_XMLNode_free(n);
      return -1;
    }
    lc_client__card_nodes=n;
    /*GWEN_XMLNode_WriteFile(n, "/tmp/cards", GWEN_XML_FLAGS_DEFAULT);*/

    /* load app XML files */
    n=GWEN_XMLNode_new(GWEN_XMLNodeTypeTag, "apps");
    if (LC_Client_ReadXmlFiles(n, "apps", "app")) {
      DBG_ERROR(LC_LOGDOMAIN, "Could not read app files");
      GWEN_XMLNode_free(n);
      return -1;
    }
    lc_client__app_nodes=n;
    /*GWEN_XMLNode_WriteFile(n, "/tmp/apps", GWEN_XML_FLAGS_DEFAULT);*/
  }

  lc_client__initcounter++;
  return 0;
}



void LC_Client_FiniCommon() {
  if (lc_client__initcounter==1) {
    GWEN_DB_Group_free(lc_client__config);
    lc_client__config=0;
    GWEN_XMLNode_free(lc_client__app_nodes);
    lc_client__app_nodes=0;
    GWEN_XMLNode_free(lc_client__card_nodes);
    lc_client__card_nodes=0;
    GWEN_Logger_Close(LC_LOGDOMAIN);
    GWEN_Fini();
  }
  if (lc_client__initcounter>0)
    lc_client__initcounter--;
}




LC_CLIENT *LC_BaseClient_new(const char *ioTypeName,
                             const char *programName,
                             const char *programVersion) {
  LC_CLIENT *cl;

  assert(programName);
  assert(programVersion);

  if (LC_Client_InitCommon()) {
    DBG_ERROR(0, "Unable to initialize, aborting");
    return 0;
  }

  GWEN_NEW_OBJECT(LC_CLIENT, cl);
  GWEN_INHERIT_INIT(LC_CLIENT, cl);
  cl->ioTypeName=strdup(ioTypeName);
  cl->programName=strdup(programName);
  cl->programVersion=strdup(programVersion);

  cl->cardNodes=lc_client__card_nodes;
  cl->appNodes=lc_client__app_nodes;
  cl->msgEngine=LC_MsgEngine_new();
  cl->monitor=LCM_Monitor_new();

  cl->dbConfig=lc_client__config;

  return cl;
}



void LC_Client_free(LC_CLIENT *cl) {
  if (cl) {
    if (cl->openCardCount) {
      DBG_ERROR(LC_LOGDOMAIN, "Still %d cards in use at LC_Client_free!",
                cl->openCardCount);
    }

    GWEN_INHERIT_FINI(LC_CLIENT, cl);
    free(cl->programVersion);
    free(cl->programName);
    free(cl->ioTypeName);
    GWEN_MsgEngine_free(cl->msgEngine);
    LCM_Monitor_free(cl->monitor);

    GWEN_FREE_OBJECT(cl);

    LC_Client_FiniCommon();
  }
}





/* _________________________________________________________________________
 * AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
 * I                    Setters for virtual functions                      I
 * YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY
 */


LC_CLIENT_INIT_FN LC_Client_SetInitFn(LC_CLIENT *cl, LC_CLIENT_INIT_FN fn) {
  LC_CLIENT_INIT_FN oldFn;

  assert(cl);
  oldFn=cl->initFn;
  cl->initFn=fn;
  return oldFn;
}



LC_CLIENT_FINI_FN LC_Client_SetFiniFn(LC_CLIENT *cl, LC_CLIENT_FINI_FN fn) {
  LC_CLIENT_FINI_FN oldFn;

  assert(cl);
  oldFn=cl->finiFn;
  cl->finiFn=fn;
  return oldFn;
}



LC_CLIENT_SETNOTIFY_FN
LC_Client_SetSetNotifyFn(LC_CLIENT *cl, LC_CLIENT_SETNOTIFY_FN fn) {
  LC_CLIENT_SETNOTIFY_FN oldFn;

  assert(cl);
  oldFn=cl->setNotifyFn;
  cl->setNotifyFn=fn;
  return oldFn;
}



LC_CLIENT_START_FN LC_Client_SetStartFn(LC_CLIENT *cl, LC_CLIENT_START_FN fn){
  LC_CLIENT_START_FN oldFn;

  assert(cl);
  oldFn=cl->startFn;
  cl->startFn=fn;
  return oldFn;
}



LC_CLIENT_STOP_FN LC_Client_SetStopFn(LC_CLIENT *cl, LC_CLIENT_STOP_FN fn) {
  LC_CLIENT_STOP_FN oldFn;

  assert(cl);
  oldFn=cl->stopFn;
  cl->stopFn=fn;
  return oldFn;
}



LC_CLIENT_GETNEXTCARD_FN
LC_Client_SetGetNextCardFn(LC_CLIENT *cl, LC_CLIENT_GETNEXTCARD_FN fn) {
  LC_CLIENT_GETNEXTCARD_FN oldFn;

  assert(cl);
  oldFn=cl->getNextCardFn;
  cl->getNextCardFn=fn;
  return oldFn;
}



LC_CLIENT_RELEASECARD_FN
LC_Client_SetReleaseCardFn(LC_CLIENT *cl, LC_CLIENT_RELEASECARD_FN fn) {
  LC_CLIENT_RELEASECARD_FN oldFn;

  assert(cl);
  oldFn=cl->releaseCardFn;
  cl->releaseCardFn=fn;
  return oldFn;
}



LC_CLIENT_EXECAPDU_FN
LC_Client_SetExecApduFn(LC_CLIENT *cl, LC_CLIENT_EXECAPDU_FN fn) {
  LC_CLIENT_EXECAPDU_FN oldFn;

  assert(cl);
  oldFn=cl->execApduFn;
  cl->execApduFn=fn;
  return oldFn;
}



LC_CLIENT_RECV_NOTIFICATION_FN
LC_Client_SetRecvNotificationFn(LC_CLIENT *cl,
                                LC_CLIENT_RECV_NOTIFICATION_FN fn) {
  LC_CLIENT_RECV_NOTIFICATION_FN oldFn;

  assert(cl);
  oldFn=cl->recvNotificationFn;
  cl->recvNotificationFn=fn;
  return oldFn;
}






/* _________________________________________________________________________
 * AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
 * I                         Virtual functions                             I
 * YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY
 */


LC_CLIENT_RESULT LC_Client_Init(LC_CLIENT *cl) {
  LC_CLIENT_RESULT res;

  /* read my own stuff from the config file */
  cl->shortTimeout=GWEN_DB_GetIntValue(cl->dbConfig, "shortTimeout", 0,
                                       LC_DEFAULT_SHORT_TIMEOUT);
  cl->longTimeout=GWEN_DB_GetIntValue(cl->dbConfig, "longTimeout", 0,
                                      LC_DEFAULT_LONG_TIMEOUT);
  cl->veryLongTimeout=GWEN_DB_GetIntValue(cl->dbConfig, "veryLongTimeout", 0,
					  LC_DEFAULT_VERY_LONG_TIMEOUT);

  /* let higher level init from DB */
  if (cl->initFn)
    res=cl->initFn(cl, cl->dbConfig);
  else
    res=LC_Client_ResultOk;

  return res;
}



LC_CLIENT_RESULT LC_Client_Fini(LC_CLIENT *cl) {
  LC_CLIENT_RESULT res;

  assert(cl);
  if (cl->openCardCount) {
    DBG_WARN(LC_LOGDOMAIN, "Still %d cards in use at LC_Client_Fini!",
             cl->openCardCount);
  }
  if (cl->finiFn)
    res=cl->finiFn(cl);
  else
    res=LC_Client_ResultOk;

  return res;
}



LC_CLIENT_RESULT LC_Client_SetNotify(LC_CLIENT *cl,
                                     GWEN_TYPE_UINT32 flags) {
  LC_CLIENT_RESULT res;

  assert(cl);
  if (cl->setNotifyFn)
    res=cl->setNotifyFn(cl, flags);
  else
    res=LC_Client_ResultNotSupported;

  return res;
}



LC_CLIENT_RESULT LC_Client_Start(LC_CLIENT *cl) {
  LC_CLIENT_RESULT res;

  assert(cl);
  if (cl->startFn)
    res=cl->startFn(cl);
  else
    res=LC_Client_ResultOk;

  return res;
}



LC_CLIENT_RESULT LC_Client_Stop(LC_CLIENT *cl) {
  LC_CLIENT_RESULT res;

  assert(cl);
  if (cl->stopFn)
    res=cl->stopFn(cl);
  else
    res=LC_Client_ResultOk;

  return res;
}



LC_CLIENT_RESULT LC_Client_GetNextCard(LC_CLIENT *cl,
                                       LC_CARD **pCard,
                                       int timeout) {
  LC_CLIENT_RESULT res;

  assert(cl);
  if (cl->getNextCardFn)
    res=cl->getNextCardFn(cl, pCard, timeout);
  else
    res=LC_Client_ResultNotSupported;

  if (res==LC_Client_ResultOk && *pCard) {
    cl->openCardCount++;
    LC_Card_SetConnected(*pCard, 1);
  }

  return res;
}



LC_CLIENT_RESULT LC_Client_ReleaseCard(LC_CLIENT *cl, LC_CARD *card) {
  LC_CLIENT_RESULT res;

  assert(cl);
  assert(card);
  if (cl->openCardCount<1) {
    DBG_ERROR(LC_LOGDOMAIN, "Released more cards then in use, aborting.");
    assert(0);
  }
  if (cl->releaseCardFn)
    res=cl->releaseCardFn(cl, card);
  else
    res=LC_Client_ResultOk;

  if (res==LC_Client_ResultOk)
    cl->openCardCount--;

  LC_Card_SetConnected(card, 0);
  return res;
}



LC_CLIENT_RESULT LC_Client_ExecApdu(LC_CLIENT *cl,
                                    LC_CARD *card,
                                    const char *apdu,
                                    unsigned int len,
                                    GWEN_BUFFER *rbuf,
                                    LC_CLIENT_CMDTARGET t,
                                    int timeout) {
  LC_CLIENT_RESULT res;

  assert(cl);

  DBG_INFO(LC_LOGDOMAIN, "Sending:");
  GWEN_Text_LogString(apdu, len, LC_LOGDOMAIN,
                      GWEN_LoggerLevelInfo);

  if (cl->execApduFn)
    res=cl->execApduFn(cl, card, apdu, len, rbuf, t, timeout);
  else
    res=LC_Client_ResultNotSupported;
  if (res==LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "Received:");
    GWEN_Text_LogString(GWEN_Buffer_GetStart(rbuf),
                        GWEN_Buffer_GetUsedBytes(rbuf),
                        LC_LOGDOMAIN,
                        GWEN_LoggerLevelInfo);
  }
  return res;
}






/* _________________________________________________________________________
 * AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
 * I                     Informational functions                           I
 * YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY
 */


const char *LC_Client_GetProgramName(const LC_CLIENT *cl) {
  assert(cl);
  return cl->programName;
}



const char *LC_Client_GetProgramVersion(const LC_CLIENT *cl) {
  assert(cl);
  return cl->programVersion;
}



int LC_Client_GetShortTimeout(const LC_CLIENT *cl) {
  assert(cl);
  return cl->shortTimeout;
}



int LC_Client_GetLongTimeout(const LC_CLIENT *cl) {
  assert(cl);
  return cl->longTimeout;
}



int LC_Client_GetVeryLongTimeout(const LC_CLIENT *cl) {
  assert(cl);
  return cl->veryLongTimeout;
}



GWEN_XMLNODE *LC_Client_GetAppNodes(const LC_CLIENT *cl) {
  assert(cl);
  return cl->appNodes;
}



GWEN_XMLNODE *LC_Client_GetCardNodes(const LC_CLIENT *cl) {
  assert(cl);
  return cl->cardNodes;
}



GWEN_MSGENGINE *LC_Client_GetMsgEngine(const LC_CLIENT *cl) {
  assert(cl);
  return cl->msgEngine;
}



GWEN_DB_NODE *LC_Client_GetReaderConfig(const LC_CLIENT *cl,
                                        const char *reader) {
  GWEN_DB_NODE *db;

  assert(cl);
  db=cl->dbConfig;
  if (!db)
    return 0;
  db=GWEN_DB_GetGroup(db, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "readerTypes");
  if (!db)
    return 0;
  db=GWEN_DB_FindFirstGroup(db, "readerType");
  while(db) {
    const char *s;

    s=GWEN_DB_GetCharValue(db, "readerType", 0, 0);
    if (s && strcasecmp(s, reader)==0)
      break;
    db=GWEN_DB_FindNextGroup(db, "readerType");
  }

  return db;
}



GWEN_DB_NODE *LC_Client_GetConfig(const LC_CLIENT *cl) {
  assert(cl);
  return cl->dbConfig;
}



LCM_MONITOR *LC_Client_GetMonitor(const LC_CLIENT *cl) {
  assert(cl);
  return cl->monitor;
}



int LC_Client_HandleNotification(LC_CLIENT *cl, const LC_NOTIFICATION *n) {
  int rv;

  assert(cl);
  assert(cl->monitor);
  rv=LCM_Monitor_HandleNotification(cl->monitor, n);
  if (rv!=-1 && cl->recvNotificationFn)
    cl->recvNotificationFn(cl, n);
  return rv;
}



const char *LC_Client_GetIoTypeName(const LC_CLIENT *cl) {
  assert(cl);
  return cl->ioTypeName;
}



#include "client_xml.c"
#include "client_cmd.c"



