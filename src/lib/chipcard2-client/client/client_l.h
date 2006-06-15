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


#ifndef CHIPCARD_CLIENT_CLIENT_L_H
#define CHIPCARD_CLIENT_CLIENT_L_H


#include <chipcard2-client/client/client.h>


int LC_Client_SelectApp(LC_CLIENT *cl,
                        LC_CARD *cd,
                        const char *appName);


GWEN_TYPE_UINT32 LC_Client_SendStopWait(LC_CLIENT *cl);
LC_CLIENT_RESULT
  LC_Client_CheckStopWait(LC_CLIENT *cl,
                          GWEN_TYPE_UINT32 rid);


GWEN_TYPE_UINT32 LC_Client_SendCommandCard(LC_CLIENT *cl,
                                           LC_CARD *cd,
                                           const char *apdu,
                                           unsigned int len,
                                           LC_CLIENT_CMDTARGET t);
LC_CLIENT_RESULT LC_Client_CheckCommandCard(LC_CLIENT *cl,
                                            GWEN_TYPE_UINT32 rid,
                                            GWEN_BUFFER *data);


GWEN_TYPE_UINT32 LC_Client_SendSetNotify(LC_CLIENT *cl,
                                         GWEN_TYPE_UINT32 flags);
LC_CLIENT_RESULT LC_Client_CheckSetNotify(LC_CLIENT *cl,
                                          GWEN_TYPE_UINT32 rid);



GWEN_TYPE_UINT32 LC_Client_SendGetDriverVar(LC_CLIENT *cl,
                                            LC_CARD *cd,
                                            const char *vname);

LC_CLIENT_RESULT LC_Client_CheckGetDriverVar(LC_CLIENT *cl,
                                             GWEN_TYPE_UINT32 rid,
                                             GWEN_BUFFER *vbuf);


GWEN_TYPE_UINT32 LC_Client_SendCardCheck(LC_CLIENT *cl,
                                         LC_CARD *cd);

LC_CLIENT_RESULT LC_Client_CheckCardCheck(LC_CLIENT *cl,
                                          GWEN_TYPE_UINT32 rid);

GWEN_TYPE_UINT32 LC_Client_SendCardReset(LC_CLIENT *cl, LC_CARD *cd);
LC_CLIENT_RESULT LC_Client_CheckCardReset(LC_CLIENT *cl,
                                          GWEN_TYPE_UINT32 rid);


GWEN_TYPE_UINT32 LC_Client_SendOpenService(LC_CLIENT *cl,
                                           GWEN_TYPE_UINT32 serverId,
                                           GWEN_TYPE_UINT32 svid,
                                           GWEN_DB_NODE *dbData);

LC_CLIENT_RESULT LC_Client_CheckOpenService(LC_CLIENT *cl,
                                            GWEN_TYPE_UINT32 rid);


GWEN_TYPE_UINT32 LC_Client_SendCloseService(LC_CLIENT *cl,
                                            GWEN_TYPE_UINT32 serverId,
                                            GWEN_TYPE_UINT32 svid,
                                            GWEN_DB_NODE *dbData);

LC_CLIENT_RESULT LC_Client_CheckCloseService(LC_CLIENT *cl,
                                             GWEN_TYPE_UINT32 rid);


GWEN_TYPE_UINT32 LC_Client_SendServiceCommand(LC_CLIENT *cl,
                                              GWEN_TYPE_UINT32 serverId,
                                              GWEN_TYPE_UINT32 svid,
                                              GWEN_DB_NODE *dbData);

LC_CLIENT_RESULT LC_Client_CheckServiceCommand(LC_CLIENT *cl,
                                               GWEN_TYPE_UINT32 rid,
                                               GWEN_DB_NODE *dbCmdResp);

GWEN_TYPE_UINT32 LC_Client_SendLockReader(LC_CLIENT *cl,
                                          GWEN_TYPE_UINT32 serverId,
                                          GWEN_TYPE_UINT32 readerId);

LC_CLIENT_RESULT LC_Client_CheckLockReader(LC_CLIENT *cl,
                                           GWEN_TYPE_UINT32 rid,
                                           GWEN_TYPE_UINT32 *lockId);

GWEN_TYPE_UINT32 LC_Client_SendUnlockReader(LC_CLIENT *cl,
                                            GWEN_TYPE_UINT32 serverId,
                                            GWEN_TYPE_UINT32 readerId,
                                            GWEN_TYPE_UINT32 lockId);

LC_CLIENT_RESULT LC_Client_CheckUnlockReader(LC_CLIENT *cl,
                                             GWEN_TYPE_UINT32 rid);

LC_CLIENT_RESULT LC_Client_GetDriverVar(LC_CLIENT *cl,
                                        LC_CARD *card,
                                        const char *vname,
                                        GWEN_BUFFER *vbuf);

LC_CLIENT_RESULT LC_Client_CardCheck(LC_CLIENT *cl,
                                     LC_CARD *card);

LC_CLIENT_RESULT LC_Client_CardReset(LC_CLIENT *cl,
                                     LC_CARD *card);


#endif /* CHIPCARD_CLIENT_CLIENT_L_H */
