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


/** @addtogroup chipcardc_client_cd
 *
 */
/*@{*/

#ifndef CHIPCARD_CLIENT_CLIENT_IMP_H
#define CHIPCARD_CLIENT_CLIENT_IMP_H

#include "client.h"

#include <chipcard/client/notifications.h>

#include <gwenhywfar/db.h>
#include <gwenhywfar/buffer.h>


typedef
  LC_CLIENT_RESULT CHIPCARD_CB (*LC_CLIENT_INIT_FN)(LC_CLIENT *cl,
                                                    GWEN_DB_NODE *dbConfig);
typedef LC_CLIENT_RESULT CHIPCARD_CB (*LC_CLIENT_FINI_FN)(LC_CLIENT *cl);

typedef LC_CLIENT_RESULT CHIPCARD_CB (*LC_CLIENT_START_FN)(LC_CLIENT *cl);
typedef LC_CLIENT_RESULT CHIPCARD_CB (*LC_CLIENT_STOP_FN)(LC_CLIENT *cl);

typedef
  LC_CLIENT_RESULT CHIPCARD_CB
  (*LC_CLIENT_SETNOTIFY_FN)(LC_CLIENT *cl,
                            uint32_t flags);

typedef
  LC_CLIENT_RESULT CHIPCARD_CB (*LC_CLIENT_GETNEXTCARD_FN)(LC_CLIENT *cl,
                                                           LC_CARD **pCard,
                                                           int timeout);
typedef
  LC_CLIENT_RESULT CHIPCARD_CB (*LC_CLIENT_RELEASECARD_FN)(LC_CLIENT *cl,
                                                           LC_CARD *card);
typedef
  LC_CLIENT_RESULT CHIPCARD_CB (*LC_CLIENT_EXECAPDU_FN)(LC_CLIENT *cl,
                                                        LC_CARD *card,
                                                        const char *apdu,
                                                        unsigned int len,
                                                        GWEN_BUFFER *rbuf,
                                                        LC_CLIENT_CMDTARGET t,
                                                        int timeout);

CHIPCARD_API
LC_CLIENT *LC_BaseClient_new(const char *ioTypeName,
                             const char *programName,
                             const char *programVersion);

CHIPCARD_API
GWEN_DB_NODE *LC_Client_GetConfig(const LC_CLIENT *cl);

CHIPCARD_API
int LC_Client_GetReaderAndDriverType(const LC_CLIENT *cl,
				     const char *readerName,
				     GWEN_BUFFER *driverType,
				     GWEN_BUFFER *readerType,
				     uint32_t *pReaderFlags);


CHIPCARD_API
int LC_Client_HandleNotification(LC_CLIENT *cl, const LC_NOTIFICATION *n);

CHIPCARD_API
LC_CLIENT_INIT_FN LC_Client_SetInitFn(LC_CLIENT *cl, LC_CLIENT_INIT_FN fn);

CHIPCARD_API
LC_CLIENT_FINI_FN LC_Client_SetFiniFn(LC_CLIENT *cl, LC_CLIENT_FINI_FN fn);

CHIPCARD_API
LC_CLIENT_SETNOTIFY_FN
LC_Client_SetSetNotifyFn(LC_CLIENT *cl, LC_CLIENT_SETNOTIFY_FN fn);

CHIPCARD_API
LC_CLIENT_START_FN LC_Client_SetStartFn(LC_CLIENT *cl, LC_CLIENT_START_FN fn);

CHIPCARD_API
LC_CLIENT_STOP_FN LC_Client_SetStopFn(LC_CLIENT *cl, LC_CLIENT_STOP_FN fn);

CHIPCARD_API
LC_CLIENT_GETNEXTCARD_FN
LC_Client_SetGetNextCardFn(LC_CLIENT *cl, LC_CLIENT_GETNEXTCARD_FN fn);

CHIPCARD_API
LC_CLIENT_RELEASECARD_FN
LC_Client_SetReleaseCardFn(LC_CLIENT *cl, LC_CLIENT_RELEASECARD_FN fn);

CHIPCARD_API
LC_CLIENT_EXECAPDU_FN
LC_Client_SetExecApduFn(LC_CLIENT *cl, LC_CLIENT_EXECAPDU_FN fn);


/*@}*/ /* addtogroup */


#endif /* CHIPCARD_CLIENT_CLIENT_IMP_H */



