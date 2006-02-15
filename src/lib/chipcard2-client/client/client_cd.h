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


#ifndef CHIPCARD_CLIENT_CLIENT_CD_H
#define CHIPCARD_CLIENT_CLIENT_CD_H

#include <chipcard2-client/client/client.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @addtogroup chipcardc_client_cd
 * @ingroup chipcard_client
 * @short API for Libchipcard2 card implementations
 *
 * This group contains the API of Libchipcard2 to be used by implementations
 * of new card types. Functions of this group must not be called by
 * applications.
 */
/*@{*/


GWEN_TYPE_UINT32 LC_Client_SendExecCommand(LC_CLIENT *cl,
                                           LC_CARD *cd,
                                           GWEN_DB_NODE *dbCmd);
LC_CLIENT_RESULT LC_Client_CheckExecCommand(LC_CLIENT *cl,
                                            GWEN_TYPE_UINT32 rid,
                                            GWEN_DB_NODE *dbRsp);

/*@}*/ /* defgroup */

#ifdef __cplusplus
}
#endif

#endif
