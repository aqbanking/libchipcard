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



#ifndef CHIPCARD_SERVER_CMD_COMMANDMGR_P_H
#define CHIPCARD_SERVER_CMD_COMMANDMGR_P_H

#include "commandmanager_l.h"
#include "cmdrequest_l.h"


struct LCCMD_COMMANDMANAGER {
  GWEN_XMLNODE *xmlCards;
  GWEN_XMLNODE *xmlApps;
  GWEN_MSGENGINE *msgEngine;
  LCCMD_CMDREQUEST_LIST *requestList;
};


int LCCMD_CommandManager_MergeXMLDefs(LCCMD_COMMANDMANAGER *mgr,
                                      GWEN_XMLNODE *destNode,
                                      GWEN_XMLNODE *node);
/** loads a specific card file */
int LCCMD_CommandManager__LoadCardFile(LCCMD_COMMANDMANAGER *mgr,
                                       const char *fname);

/** loads all card files from the given location and adds all folders
 * to the given string list */
int LCCMD_CommandManager___LoadCardFiles(LCCMD_COMMANDMANAGER *mgr,
                                         const char *where,
                                         GWEN_STRINGLIST *folders);

/* loads all card files from the given path */
int LCCMD_CommandManager__LoadCardFiles(LCCMD_COMMANDMANAGER *mgr,
                                        const char *where);
/** loads all card files from all paths of @ref LCS_PATH_DRIVER_INFODIR */
int LCCMD_CommandManager__LoadAllCards(LCCMD_COMMANDMANAGER *mgr);

int LCCMD_CommandManager__AddCardTypesByAtr(LCCMD_COMMANDMANAGER *mgr,
                                            LCCO_CARD *card);

GWEN_XMLNODE *LCCMD_CommandManager___FindCommand(GWEN_XMLNODE *node,
                                                 const char *commandName,
                                                 const char *driverType,
                                                 const char *readerType);
GWEN_XMLNODE *LCCMD_CardCommander__FindCommand(LCCMD_COMMANDMANAGER *mgr,
                                               GWEN_XMLNODE *node,
                                               const char *commandName,
                                               const char *driverType,
                                               const char *readerType);



GWEN_XMLNODE *LCCMD_CommandManager___FindResult(GWEN_XMLNODE *cmd,
                                                int sw1,
                                                int sw2);
GWEN_XMLNODE *LCCMD_CommandManager__FindResult(LCCMD_COMMANDMANAGER *mgr,
                                               LCCO_CARD *card,
                                               GWEN_XMLNODE *cmd,
                                               int sw1,
                                               int sw2);

GWEN_XMLNODE *LCCMD_CommandManager__FindResponse(LCCMD_COMMANDMANAGER *mgr,
                                                 GWEN_XMLNODE *cmd,
                                                 const char *typ);


int LCCMD_CommandManager__BuildApdu(LCCMD_COMMANDMANAGER *mgr,
                                    GWEN_XMLNODE *node,
                                    GWEN_DB_NODE *cmdData,
                                    GWEN_BUFFER *gbuf);

int LCCMD_CommandManager__ParseResult(LCCMD_COMMANDMANAGER *mgr,
                                      LCCO_CARD *card,
                                      GWEN_XMLNODE *node,
                                      GWEN_BUFFER *gbuf,
                                      GWEN_DB_NODE *rspData);

int LCCMD_CommandManager__ParseResponse(LCCMD_COMMANDMANAGER *mgr,
                                        LCCO_CARD *card,
                                        GWEN_XMLNODE *node,
                                        GWEN_BUFFER *gbuf,
                                        GWEN_DB_NODE *rspData);

int LCCMD_CommandManager__ParseAnswer(LCCMD_COMMANDMANAGER *mgr,
                                      LCCO_CARD *card,
                                      LCCMD_CMDREQUEST *req,
                                      GWEN_BUFFER *gbuf,
                                      GWEN_DB_NODE *rspData);





#endif /* CHIPCARD_SERVER_CMD_COMMANDMGR_P_H */

