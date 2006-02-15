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


#ifndef CHIPCARD_CLIENT_CLIENT_SV_H
#define CHIPCARD_CLIENT_CLIENT_SV_H

#include <chipcard2-client/client/client_cd.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @addtogroup chipcardc_client_sv
 * @ingroup chipcard_client
 * @short API for Libchipcard2 card implementations
 *
 * This group contains the API of Libchipcard2 to be used by implementations
 * of new service types. Functions of this group must not be called by
 * applications.
 */
/*@{*/


/** @name Generic Request Functions
 *
 */
/*@{*/
GWEN_TYPE_UINT32 LC_Client_SendRequest(LC_CLIENT *cl,
                                       LC_CARD *card,
				       GWEN_TYPE_UINT32 serverId,
				       GWEN_DB_NODE *dbReq);

GWEN_DB_NODE *LC_Client_GetNextResponse(LC_CLIENT *cl,
                                        GWEN_TYPE_UINT32 rqid);
GWEN_DB_NODE *LC_Client_WaitForNextResponse(LC_CLIENT *cl,
                                            GWEN_TYPE_UINT32 rqid,
                                            int timeout);
int LC_Client_CheckForError(GWEN_DB_NODE *db);
/*@}*/


/** @name Special Requests
 *
 */
/*@{*/
GWEN_TYPE_UINT32 LC_Client_SendTakeCard(LC_CLIENT *cl, LC_CARD *cd);
LC_CLIENT_RESULT LC_Client_CheckTakeCard(LC_CLIENT *cl,
                                         GWEN_TYPE_UINT32 rid);

GWEN_TYPE_UINT32 LC_Client_SendReleaseCard(LC_CLIENT *cl, LC_CARD *cd);
LC_CLIENT_RESULT LC_Client_CheckReleaseCard(LC_CLIENT *cl,
                                            GWEN_TYPE_UINT32 rid);

GWEN_TYPE_UINT32 LC_Client_SendSelectCardApp(LC_CLIENT *cl,
                                             LC_CARD *cd,
                                             const char *cardName,
                                             const char *appName);
LC_CLIENT_RESULT LC_Client_CheckSelectCardApp(LC_CLIENT *cl,
                                              GWEN_TYPE_UINT32 rid);
/*@}*/


/*@}*/ /* addtogroup */

#ifdef __cplusplus
}
#endif

#endif
