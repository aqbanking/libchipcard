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



#ifndef CHIPCARD_SERVER_CMD_COMMANDMGR_L_H
#define CHIPCARD_SERVER_CMD_COMMANDMGR_L_H

#include <gwenhywfar/types.h>
#include <gwenhywfar/xml.h>
#include <gwenhywfar/stringlist.h>
#include <gwenhywfar/db.h>
#include <gwenhywfar/buffer.h>
#include <gwenhywfar/msgengine.h>


typedef struct LCCMD_COMMANDMANAGER LCCMD_COMMANDMANAGER;

#include <chipcard2/chipcard2.h>
#include <chipcard2-server/common/card.h>


LCCMD_COMMANDMANAGER *LCCMD_CommandManager_new();
void LCCMD_CommandManager_free(LCCMD_COMMANDMANAGER *mgr);

int LCCMD_CommandManager_Init(LCCMD_COMMANDMANAGER *mgr, GWEN_DB_NODE *db);
int LCCMD_CommandManager_Fini(LCCMD_COMMANDMANAGER *mgr, GWEN_DB_NODE *db);


/**
 * This function is called by the LCS_FullServer to let the command
 * manager extend the card and store some needed data with the new card.
 */
void LCCMD_CommandManager_NewCard(LCCMD_COMMANDMANAGER *clm, LCCO_CARD *card);

/**
 * Selects the given card type on the card. This is used to locate commands
 * for @ref LCCMD_CommandManager_BuildCommand.
 */
int LCCMD_CommandManager_SelectCardType(LCCMD_COMMANDMANAGER *mgr,
                                        LCCO_CARD *card,
                                        const char *cardName);

/**
 * Creates an APDU from the command data given for the currently selected card
 * type.
 */
int LCCMD_CommandManager_BuildCommand(LCCMD_COMMANDMANAGER *mgr,
                                      LCCO_CARD *card,
                                      const char *cmdName,
                                      GWEN_DB_NODE *dbCmd,
                                      GWEN_BUFFER *apdu,
                                      const char **target,
                                      GWEN_TYPE_UINT32 *rqid);

/**
 * Parses the given APDU response and creates some vars in dbRsp.
 * If the APDU yielded some response data then the last two bytes are
 * removed from this data (they always contain the result bytes SW1 and SW2).
 */
int LCCMD_CommandManager_ParseAnswer(LCCMD_COMMANDMANAGER *mgr,
                                     LCCO_CARD *card,
                                     GWEN_TYPE_UINT32 rqid,
                                     GWEN_BUFFER *gbuf,
                                     GWEN_DB_NODE *dbRsp);


#endif /* CHIPCARD_SERVER_CMD_COMMANDMGR_L_H */

