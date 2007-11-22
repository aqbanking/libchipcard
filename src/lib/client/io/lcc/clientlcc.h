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


#ifndef CHIPCARD_CLIENT_CLIENTLCC_H
#define CHIPCARD_CLIENT_CLIENTLCC_H

#define LC_CLIENT_LCC_NAME "lcc"


#include <chipcard/client/client.h>


/** @addtogroup chipcardc_client_lcc
 *
 */
/*@{*/


typedef int (*LC_CLIENTLCC_HANDLE_REQUEST_FN)(LC_CLIENT *cl,
                                              uint32_t rid,
                                              const char *name,
                                              GWEN_DB_NODE *dbRequest);


CHIPCARD_API
LC_CLIENT *LC_ClientLcc_new(const char *programName,
                            const char *programVersion);



CHIPCARD_API
int LC_ClientLcc_DeleteRequest(LC_CLIENT *cl, uint32_t rqid);




CHIPCARD_API
LC_CARD *LC_ClientLcc_PeekNextCard(LC_CLIENT *cl);

CHIPCARD_API
LC_CARD *LC_ClientLcc_GetNextCard(LC_CLIENT *cl);

CHIPCARD_API
LC_CLIENT_RESULT LC_ClientLcc_WaitForNextCard(LC_CLIENT *cl,
                                              LC_CARD **pCard,
                                              int timeout);



/* @name Request: StartWait
 *
 */
/*@{*/
CHIPCARD_API
uint32_t LC_ClientLcc_SendStartWait(LC_CLIENT *cl,
                                            uint32_t rflags,
                                            uint32_t rmask);

CHIPCARD_API
LC_CLIENT_RESULT
LC_ClientLcc_CheckStartWait(LC_CLIENT *cl,
                            uint32_t rid);

CHIPCARD_API
LC_CLIENT_RESULT LC_ClientLcc_StartWait(LC_CLIENT *cl,
                                        uint32_t rflags,
                                        uint32_t rmask);
/*@}*/




/* @name Request: StopWait
 *
 */
/*@{*/
CHIPCARD_API
uint32_t LC_ClientLcc_SendStopWait(LC_CLIENT *cl);


CHIPCARD_API
LC_CLIENT_RESULT
LC_ClientLcc_CheckStopWait(LC_CLIENT *cl,
                           uint32_t rid);

CHIPCARD_API
LC_CLIENT_RESULT LC_ClientLcc_StopWait(LC_CLIENT *cl);
/*@}*/



/* @name Request: TakeCard
 *
 */
/*@{*/
CHIPCARD_API
uint32_t LC_ClientLcc_SendTakeCard(LC_CLIENT *cl, LC_CARD *cd);

CHIPCARD_API
LC_CLIENT_RESULT
LC_ClientLcc_CheckTakeCard(LC_CLIENT *cl,
                           uint32_t rid);

CHIPCARD_API
LC_CLIENT_RESULT LC_ClientLcc_TakeCard(LC_CLIENT *cl, LC_CARD *cd);
/*@}*/



/* @name Request: ReleaseCard
 *
 */
/*@{*/
CHIPCARD_API
uint32_t LC_ClientLcc_SendReleaseCard(LC_CLIENT *cl, LC_CARD *cd);

CHIPCARD_API
LC_CLIENT_RESULT
LC_ClientLcc_CheckReleaseCard(LC_CLIENT *cl,
                              uint32_t rid);

CHIPCARD_API
LC_CLIENT_RESULT LC_ClientLcc_ReleaseCard(LC_CLIENT *cl, LC_CARD *cd);
/*@}*/



/* @name Request: CommandCard
 *
 */
/*@{*/

CHIPCARD_API
uint32_t LC_ClientLcc_SendCommandCard(LC_CLIENT *cl,
                                              LC_CARD *cd,
                                              const char *apdu,
                                              unsigned int len,
                                              LC_CLIENT_CMDTARGET t);

CHIPCARD_API
LC_CLIENT_RESULT
LC_ClientLcc_CheckCommandCard(LC_CLIENT *cl,
                              uint32_t rid,
                              GWEN_BUFFER *data);

CHIPCARD_API
LC_CLIENT_RESULT LC_ClientLcc_CommandCard(LC_CLIENT *cl,
                                          LC_CARD *card,
                                          const char *apdu,
                                          unsigned int len,
                                          GWEN_BUFFER *rbuf,
                                          LC_CLIENT_CMDTARGET t,
                                          int timeout);
/*@}*/



/* @name Request: CommandCard
 *
 */
/*@{*/

CHIPCARD_API
uint32_t LC_ClientLcc_SendSetNotify(LC_CLIENT *cl,
                                            uint32_t flags);

CHIPCARD_API
LC_CLIENT_RESULT LC_ClientLcc_CheckSetNotify(LC_CLIENT *cl,
                                             uint32_t rid);

CHIPCARD_API
LC_CLIENT_RESULT LC_ClientLcc_SetNotify(LC_CLIENT *cl,
                                        uint32_t flags);



/*@}*/




/** @name Working With Readers
 *
 */
/*@{*/

CHIPCARD_API
uint32_t LC_ClientLcc_SendLockReader(LC_CLIENT *cl,
                                             uint32_t serverId,
                                             uint32_t readerId);

CHIPCARD_API
LC_CLIENT_RESULT LC_ClientLcc_CheckLockReader(LC_CLIENT *cl,
                                              uint32_t rid,
                                              uint32_t *lockId);

CHIPCARD_API
LC_CLIENT_RESULT LC_ClientLcc_LockReader(LC_CLIENT *cl,
                                         uint32_t serverId,
                                         uint32_t readerId,
                                         uint32_t *lockId);


CHIPCARD_API
uint32_t LC_ClientLcc_SendUnlockReader(LC_CLIENT *cl,
                                               uint32_t serverId,
                                               uint32_t readerId,
                                               uint32_t lockId);

CHIPCARD_API
LC_CLIENT_RESULT LC_ClientLcc_CheckUnlockReader(LC_CLIENT *cl,
                                                uint32_t rid);

CHIPCARD_API
LC_CLIENT_RESULT LC_ClientLcc_UnlockReader(LC_CLIENT *cl,
                                           uint32_t serverId,
                                           uint32_t readerId,
                                           uint32_t lockId);


CHIPCARD_API
uint32_t LC_ClientLcc_SendReaderCommand(LC_CLIENT *cl,
                                                uint32_t serverId,
                                                uint32_t readerId,
                                                uint32_t lockId,
                                                GWEN_DB_NODE *dbData);

CHIPCARD_API
LC_CLIENT_RESULT LC_ClientLcc_CheckReaderCommand(LC_CLIENT *cl,
                                                 uint32_t rid,
                                                 GWEN_DB_NODE *dbCmdResp);

CHIPCARD_API
LC_CLIENT_RESULT LC_ClientLcc_ReaderCommand(LC_CLIENT *cl,
                                            uint32_t serverId,
                                            uint32_t readerId,
                                            uint32_t lockId,
                                            GWEN_DB_NODE *dbData,
                                            GWEN_DB_NODE *dbCmdResp);


/*@}*/




/* @name Monitoring System
 *
 */
/*@{*/
CHIPCARD_API
uint32_t LC_ClientLcc_SendSetNotify(LC_CLIENT *cl,
                                            uint32_t flags);

CHIPCARD_API
LC_CLIENT_RESULT LC_ClientLcc_CheckSetNotify(LC_CLIENT *cl,
                                             uint32_t rid);

CHIPCARD_API
LC_CLIENT_RESULT LC_ClientLcc_SetNotify(LC_CLIENT *cl,
                                        uint32_t flags);


/*@}*/


CHIPCARD_API
int LC_ClientLcc_Work(LC_CLIENT *cl, int maxmsg);

CHIPCARD_API
LC_CLIENT_RESULT LC_ClientLcc_Work_Wait(LC_CLIENT *cl, int timeout);



/*@}*/




#endif /* CHIPCARD_CLIENT_CLIENTLCC_H */

