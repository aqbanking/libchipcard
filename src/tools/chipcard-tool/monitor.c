/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: listreaders.c 219 2006-09-08 12:57:36Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "global.h"
#include <time.h>
#include <assert.h>
#include <chipcard/client/io/lcc/clientlcc.h>
#include <chipcard/client/mon/monitor.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/gwentime.h>
#include <gwenhywfar/buffer.h>



void printPrelude(uint32_t serverId) {
  GWEN_TIME *ti;
  GWEN_BUFFER *dbuf;
  char numbuf[32];

  ti=GWEN_CurrentTime();
  assert(ti);
  dbuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Time_toString(ti, "YYYY/MM/DD hh:mm:ss", dbuf);
  GWEN_Buffer_AppendString(dbuf, " : Notification from server ");
  snprintf(numbuf, sizeof(numbuf), "%08x", serverId);
  GWEN_Buffer_AppendString(dbuf, numbuf);
  fprintf(stderr, "%s\n", GWEN_Buffer_GetStart(dbuf));
  GWEN_Buffer_free(dbuf);
}



void handleDriverN(LC_CLIENT *cl, const LC_NOTIFICATION *n) {
  const char *ncode;
  GWEN_DB_NODE *dbData;

  ncode=LC_Notification_GetCode(n);
  dbData=LC_Notification_GetData(n);
  if (ncode && dbData) {
    const char *dtype;
    const char *dname;
    const char *dfile;
    const char *info;

    dtype=GWEN_DB_GetCharValue(dbData, "driverType", 0, "(none)");
    dname=GWEN_DB_GetCharValue(dbData, "driverName", 0, "(none)");
    dfile=GWEN_DB_GetCharValue(dbData, "libraryFile", 0, "(none)");
    info=GWEN_DB_GetCharValue(dbData, "info", 0, 0);

    if (strcasecmp(ncode, LC_NOTIFY_CODE_DRIVER_START)==0) {
      printPrelude(LC_Notification_GetServerId(n));
      fprintf(stderr, "    Driver \"%s\" started (%s, %s)\n",
              dname, dtype, dfile);
      if (info)
        fprintf(stderr, "    %s\n", info);
    }
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_DRIVER_UP)==0) {
      printPrelude(LC_Notification_GetServerId(n));
      fprintf(stderr, "    Driver \"%s\" is up (%s)\n",
              dname, dtype);
      if (info)
        fprintf(stderr, "    %s\n", info);
    }
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_DRIVER_DOWN)==0) {
      printPrelude(LC_Notification_GetServerId(n));
      fprintf(stderr, "    Driver \"%s\" is down (%s)\n",
              dname, dtype);
      if (info)
        fprintf(stderr, "    %s\n", info);
    }
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_DRIVER_ERROR)==0) {
      printPrelude(LC_Notification_GetServerId(n));
      fprintf(stderr, "    Driver \"%s\" has an error  (%s)\n",
              dname, dtype);
      if (info)
        fprintf(stderr, "    %s\n", info);
    }
  }
}



void handleReaderN(LC_CLIENT *cl, const LC_NOTIFICATION *n) {
  const char *ncode;
  GWEN_DB_NODE *dbData;

  ncode=LC_Notification_GetCode(n);
  dbData=LC_Notification_GetData(n);
  if (ncode && dbData) {
    const char *rtype;
    const char *rname;
    const char *info;

    rtype=GWEN_DB_GetCharValue(dbData, "readerType", 0, "(none)");
    rname=GWEN_DB_GetCharValue(dbData, "readerName", 0, "(none)");
    info=GWEN_DB_GetCharValue(dbData, "info", 0, 0);

    if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_START)==0) {
      printPrelude(LC_Notification_GetServerId(n));
      fprintf(stderr, "    Reader \"%s\" started (%s)\n",
              rname, rtype);
      if (info)
        fprintf(stderr, "    %s\n", info);
    }
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_UP)==0) {
      printPrelude(LC_Notification_GetServerId(n));
      fprintf(stderr, "    Reader \"%s\" is up (%s)\n",
              rname, rtype);
      if (info)
        fprintf(stderr, "    %s\n", info);
    }
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_DOWN)==0) {
      printPrelude(LC_Notification_GetServerId(n));
      fprintf(stderr, "    Reader \"%s\" is down (%s)\n",
              rname, rtype);
      if (info)
        fprintf(stderr, "    %s\n", info);
    }
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_ERROR)==0) {
      printPrelude(LC_Notification_GetServerId(n));
      fprintf(stderr, "    Reader \"%s\" has an error  (%s)\n",
              rname, rtype);
      if (info)
        fprintf(stderr, "    %s\n", info);
    }
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_ADD)==0) {
      printPrelude(LC_Notification_GetServerId(n));
      fprintf(stderr, "    Reader \"%s\" added (%s)\n",
              rname, rtype);
      if (info)
        fprintf(stderr, "    %s\n", info);
    }
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_DEL)==0) {
      printPrelude(LC_Notification_GetServerId(n));
      fprintf(stderr, "    Reader \"%s\" removed (%s)\n",
              rname, rtype);
      if (info)
        fprintf(stderr, "    %s\n", info);
    }
  }
}



void handleCardN(LC_CLIENT *cl, const LC_NOTIFICATION *n) {
  const char *ncode;
  GWEN_DB_NODE *dbData;

  ncode=LC_Notification_GetCode(n);
  dbData=LC_Notification_GetData(n);
  if (ncode && dbData) {
    uint32_t rid=0;
    int slotNum=0;
    uint32_t cardNum=0;
    const char *s;
    const char *info=0;

    s=GWEN_DB_GetCharValue(dbData, "readerId", 0, 0);
    if (s)
      sscanf(s, "%x", &rid);
    s=GWEN_DB_GetCharValue(dbData, "slotNum", 0, 0);
    if (s)
      sscanf(s, "%i", &slotNum);
    s=GWEN_DB_GetCharValue(dbData, "cardNum", 0, 0);
    if (s)
      sscanf(s, "%i", &cardNum);

    info=GWEN_DB_GetCharValue(dbData, "info", 0, 0);

    if (strcasecmp(ncode, LC_NOTIFY_CODE_CARD_INSERTED)==0) {
      printPrelude(LC_Notification_GetServerId(n));
      fprintf(stderr, "    Card \"%08x/%d/%d\" inserted\n",
              rid, slotNum, cardNum);
      if (info)
        fprintf(stderr, "    %s\n", info);
    }
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_CARD_REMOVED)==0) {
      printPrelude(LC_Notification_GetServerId(n));
      fprintf(stderr, "    Card \"%08x/%d/%d\" removed\n",
              rid, slotNum, cardNum);
      if (info)
        fprintf(stderr, "    %s\n", info);
    }
  }
}



void recvNotification(LC_CLIENT *cl, const LC_NOTIFICATION *n) {
  const char *ntype;

  ntype=LC_Notification_GetType(n);
  if (ntype) {
    if (strcasecmp(ntype, LC_NOTIFY_TYPE_DRIVER)==0)
      handleDriverN(cl, n);
    else if (strcasecmp(ntype, LC_NOTIFY_TYPE_READER)==0)
      handleReaderN(cl, n);
    else if (strcasecmp(ntype, LC_NOTIFY_TYPE_CARD)==0)
      handleCardN(cl, n);
    else {
      const char *ncode;
      GWEN_DB_NODE *dbData;

      ncode=LC_Notification_GetCode(n);
      printPrelude(LC_Notification_GetServerId(n));
      fprintf(stderr, "    Unknown notification: %s/%s\n",
              ntype, ncode);
      dbData=LC_Notification_GetData(n);
      if (dbData)
        GWEN_DB_Dump(dbData, stderr, 4);
    }
  }
}



int monitor(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs){
  LC_CLIENT_RESULT res;
  int verbosity;
  const char *s;

  s=LC_Client_GetIoTypeName(cl);
  assert(s);
  if (strcasecmp(s, LC_CLIENT_LCC_NAME)!=0) {
    DBG_ERROR(LC_LOGDOMAIN,
              "This command only works with the "
              "libchipcard3 ressource manager (%s), but "
              "you are using \"%s\"",
              LC_CLIENT_LCC_NAME, s);
    return 2;
  }

  LC_Client_SetRecvNotificationFn(cl, recvNotification);
  verbosity=GWEN_DB_GetIntValue(dbArgs, "verbosity", 0, 0);
  if (verbosity)
    fprintf(stderr, "Contacting server(s)...\n");
  res=LC_Client_SetNotify(cl,
                          LC_NOTIFY_FLAGS_READER_START |
                          LC_NOTIFY_FLAGS_READER_UP    |
                          LC_NOTIFY_FLAGS_READER_DOWN  |
                          LC_NOTIFY_FLAGS_READER_ERROR |
                          LC_NOTIFY_FLAGS_READER_ADD   |
                          LC_NOTIFY_FLAGS_READER_DEL   |
                          LC_NOTIFY_FLAGS_DRIVER_START |
                          LC_NOTIFY_FLAGS_DRIVER_UP    |
                          LC_NOTIFY_FLAGS_DRIVER_DOWN  |
                          LC_NOTIFY_FLAGS_DRIVER_ERROR |
                          LC_NOTIFY_FLAGS_DRIVER_ADD   |
                          LC_NOTIFY_FLAGS_DRIVER_DEL   |
                          LC_NOTIFY_FLAGS_CARD_INSERTED|
                          LC_NOTIFY_FLAGS_CARD_REMOVED);
  if (res!=LC_Client_ResultOk) {
    showError(0, res, "SetNotify");
    return 2;
  }

  if (GWEN_DB_GetIntValue(dbArgs, "startAll", 0, 0)) {
    if (verbosity)
      fprintf(stderr, "Starting all readers...\n");
    res=LC_Client_Start(cl);
    if (res!=LC_Client_ResultOk) {
      showError(0, res, "Start");
      return 2;
    }
  }

  while(1) {
    if (verbosity)
      fprintf(stderr, "        (Waiting for notifications...)\n");
    res=LC_ClientLcc_Work_Wait(cl, 30);
    if (res!=LC_Client_ResultOk &&
        res!=LC_Client_ResultWait) {
      showError(0, res, "Work");
      break;
    }
  }

  return 0;
}




