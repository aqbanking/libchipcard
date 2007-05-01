/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: client_p.h 137 2005-11-03 13:07:50Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CLIENT_CLIENT_P_H
#define CHIPCARD_CLIENT_CLIENT_P_H

#include "client_l.h"
#include "client_imp.h"

#include <gwenhywfar/msgengine.h>


struct LC_CLIENT {
  GWEN_INHERIT_ELEMENT(LC_CLIENT)
  char *ioTypeName;
  char *programName;
  char *programVersion;

  GWEN_DB_NODE *dbConfig;
  int shortTimeout;
  int longTimeout;
  int veryLongTimeout;

  GWEN_MSGENGINE *msgEngine;
  GWEN_XMLNODE *cardNodes;
  GWEN_XMLNODE *appNodes;

  int openCardCount;

  LC_CLIENT_RECV_NOTIFICATION_FN recvNotificationFn;

  LC_CLIENT_INIT_FN initFn;
  LC_CLIENT_FINI_FN finiFn;
  LC_CLIENT_SETNOTIFY_FN setNotifyFn;
  LC_CLIENT_START_FN startFn;
  LC_CLIENT_STOP_FN stopFn;
  LC_CLIENT_GETNEXTCARD_FN getNextCardFn;
  LC_CLIENT_RELEASECARD_FN releaseCardFn;
  LC_CLIENT_EXECAPDU_FN execApduFn;

  LCM_MONITOR *monitor;
};


static void LC_Client__SampleXmlFiles(const char *where,
                                      GWEN_STRINGLIST *sl);

static int LC_Client_MergeXMLDefs(GWEN_XMLNODE *destNode,
                                  GWEN_XMLNODE *node);

static int LC_Client_ReadXmlFiles(GWEN_XMLNODE *root,
                                  const char *tPlural,
                                  const char *tSingular);

static GWEN_XMLNODE *LC_Client__FindCommandInCardNode(GWEN_XMLNODE *node,
                                                      const char *commandName,
                                                      const char *driverType,
                                                      const char *readerType);
static GWEN_XMLNODE*
  LC_Client_FindCommandInCardNode(GWEN_XMLNODE *node,
                                  const char *commandName,
                                  const char *driverType,
                                  const char *readerType);
static GWEN_XMLNODE*
  LC_Client_FindCommandInCardFamily(GWEN_XMLNODE *cardNodes,
                                    GWEN_STRINGLIST *handled,
                                    const char *cardType,
                                    const char *commandName,
                                    const char *driverType,
                                    const char *readerType);
static GWEN_XMLNODE*
  LC_Client_FindCommandInCardTypes(GWEN_XMLNODE *cardNodes,
                                   const GWEN_STRINGLIST *cardTypes,
                                   const char *commandName,
                                   const char *driverType,
                                   const char *readerType);


static GWEN_XMLNODE *LC_Client_FindResultInNode(GWEN_XMLNODE *node,
                                                int sw1, int sw2);

static GWEN_XMLNODE *LC_Client_FindResult(LC_CLIENT *cl,
                                          GWEN_XMLNODE *cmdNode,
                                          int sw1, int sw2);


static GWEN_XMLNODE *LC_Client_FindResponseInNode(GWEN_XMLNODE *cmd,
                                                  const char *typ);

static GWEN_XMLNODE *LC_Client_FindResponse(LC_CLIENT *cl,
                                            GWEN_XMLNODE *cmdNode,
                                            const char *typ);



static int LC_Client__BuildApdu(LC_CLIENT *cl,
                                GWEN_XMLNODE *node,
                                GWEN_DB_NODE *cmdData,
                                GWEN_BUFFER *gbuf);

static int LC_Client_ParseResult(LC_CLIENT *cl,
                                 GWEN_XMLNODE *node,
                                 GWEN_BUFFER *gbuf,
                                 GWEN_DB_NODE *rspData);

static int LC_Client_ParseResponse(LC_CLIENT *cl,
                                   GWEN_XMLNODE *node,
                                   GWEN_BUFFER *gbuf,
                                   GWEN_DB_NODE *rspData);

static int LC_Client_ParseAnswer(LC_CLIENT *cl,
                                 GWEN_XMLNODE *node,
                                 GWEN_BUFFER *gbuf,
                                 GWEN_DB_NODE *rspData);





#endif /* CHIPCARD_CLIENT_CLIENT_P_H */



