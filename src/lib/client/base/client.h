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


#ifndef CHIPCARD_CLIENT_CLIENT_H
#define CHIPCARD_CLIENT_CLIENT_H


/** @addtogroup chipcardc_client_app
 */
/*@{*/

#include <gwenhywfar/inherit.h>
#include <chipcard3/chipcard3.h>


#ifdef __cplusplus
extern "C" {
#endif

#define LC_DEFAULT_SHORT_TIMEOUT     10
#define LC_DEFAULT_LONG_TIMEOUT      30
#define LC_DEFAULT_VERY_LONG_TIMEOUT 60

#define LC_CLIENT_TIMEOUT_NONE    0
#define LC_CLIENT_TIMEOUT_FOREVER (-1)


typedef struct LC_CLIENT LC_CLIENT;
GWEN_INHERIT_FUNCTION_LIB_DEFS(LC_CLIENT, CHIPCARD_API)


/**
 * Result codes for operations.
 */
typedef enum {
  LC_Client_ResultOk=0,
  LC_Client_ResultWait,
  LC_Client_ResultIpcError,
  LC_Client_ResultCmdError,
  LC_Client_ResultDataError,
  LC_Client_ResultAborted,
  LC_Client_ResultInvalid,
  LC_Client_ResultInternal,
  LC_Client_ResultGeneric,
  LC_Client_ResultNoData,
  LC_Client_ResultCardRemoved,
  LC_Client_ResultNotSupported,
  LC_Client_ResultCfgError,
  LC_Client_ResultNotFound,
  LC_Client_ResultIoError,
  LC_Client_ResultBadPin
} LC_CLIENT_RESULT;


/**
 * Targets for commands (used by @ref LC_Card_ExecAPDU)
 */
typedef enum {
  LC_Client_CmdTargetCard=0,
  LC_Client_CmdTargetReader
} LC_CLIENT_CMDTARGET;



#ifdef __cplusplus
}
#endif


#include <chipcard3/client/card.h>
#include <chipcard3/client/notifications.h>
#include <chipcard3/client/mon/monitor.h>
#include <chipcard3/client/switch.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef void (*LC_CLIENT_RECV_NOTIFICATION_FN)(LC_CLIENT *cl,
                                               const LC_NOTIFICATION *n);



/** @name Main API
 *
 * To work with this API you'll need to create a client object first.
 * This is normally done by @ref LC_Client_new.
 */
/*@{*/

/**
 * Init Libchipcard3. This functions reads the configuration file and
 * the card command description files. It does not allocate the readers
 * (see @ref LC_Client_Start), so it is perfectly save to call this function
 * upon startup of the application.
 */
CHIPCARD_API
LC_CLIENT_RESULT LC_Client_Init(LC_CLIENT *cl);

/**
 * Deinit Libchipcard3. Unloads all data files.
 *
 */
CHIPCARD_API
LC_CLIENT_RESULT LC_Client_Fini(LC_CLIENT *cl);

/**
 * Tell the ressource manager that we are now about to access chipcards.
 * When using Libchipcard3's own resource manager then upon receiption of
 * this call the ressource manager starts connecting card readers. Only then
 * the readers are allocated. Without calling this function the other function
 * @ref LC_Client_GetNextCard will never return a card.
 */
CHIPCARD_API
LC_CLIENT_RESULT LC_Client_Start(LC_CLIENT *cl);

/**
 * Tell the resource manager that we don't need more cards. When the last
 * allocated card becomes unused the ressource manager is allowed to
 * deallocate the readers. Your application should always call this function
 * as soon as it is finished accessing cards.
 */
CHIPCARD_API
LC_CLIENT_RESULT LC_Client_Stop(LC_CLIENT *cl);

/**
 * Wait for the next card to become available. The application must call
 * @ref LC_Client_Start prior to calling this function otherwise it will
 * never return a card (because no readers are allocated).
 */
CHIPCARD_API
LC_CLIENT_RESULT LC_Client_GetNextCard(LC_CLIENT *cl,
                                       LC_CARD **pCard,
                                       int timeout);

/* This function releases the given card. After calling this function the card
 * can no longer be used for commands etc.
 * You should call @ref LC_Card_free on the card afterwards.
 */
CHIPCARD_API
LC_CLIENT_RESULT LC_Client_ReleaseCard(LC_CLIENT *cl, LC_CARD *card);

/**
 * Release all ressources associated with Libchipcard3. This must be called
 * at the end of the application to avoid memory leaks.
 */
CHIPCARD_API
void LC_Client_free(LC_CLIENT *cl);

/**
 * Returns the ressource manager type this client uses (see
 * @ref LC_Client_Factory)
 */
CHIPCARD_API
const char *LC_Client_GetIoTypeName(const LC_CLIENT *cl);

/*@}*/


/** @name Informational Functions
 *
 */
/*{@*/
CHIPCARD_API
const char *LC_Client_GetProgramName(const LC_CLIENT *cl);

CHIPCARD_API
const char *LC_Client_GetProgramVersion(const LC_CLIENT *cl);

CHIPCARD_API
int LC_Client_GetShortTimeout(const LC_CLIENT *cl);

CHIPCARD_API
int LC_Client_GetLongTimeout(const LC_CLIENT *cl);

CHIPCARD_API
int LC_Client_GetVeryLongTimeout(const LC_CLIENT *cl);
/*@}*/


/** @name Retrieve Some Internal Information
 *
 */
/*@{*/
CHIPCARD_API
GWEN_XMLNODE *LC_Client_GetAppNodes(const LC_CLIENT *cl);

CHIPCARD_API
GWEN_XMLNODE *LC_Client_GetCardNodes(const LC_CLIENT *cl);

CHIPCARD_API
GWEN_DB_NODE *LC_Client_GetReaderConfig(const LC_CLIENT *cl,
                                        const char *reader);
/*@}*/


/** @name Monitoring
 * The monitoring code of Libchipcard3 listens on and interpretes server
 * notifications. Not all resource managers support such notifications
 * (Libchipcard3's own ressource manager does).
 */
/*@{*/
CHIPCARD_API
LCM_MONITOR *LC_Client_GetMonitor(const LC_CLIENT *cl);

CHIPCARD_API
LC_CLIENT_RESULT LC_Client_SetNotify(LC_CLIENT *cl,
                                     GWEN_TYPE_UINT32 flags);

CHIPCARD_API
LC_CLIENT_RECV_NOTIFICATION_FN
LC_Client_SetRecvNotificationFn(LC_CLIENT *cl,
                                LC_CLIENT_RECV_NOTIFICATION_FN fn);
/*@}*/


#ifdef __cplusplus
}
#endif


/*@}*/

#endif /* CHIPCARD_CLIENT_CLIENT_H */



