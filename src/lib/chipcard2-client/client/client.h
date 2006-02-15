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


#ifndef CHIPCARD_CLIENT_CLIENT_H
#define CHIPCARD_CLIENT_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gwenhywfar/inherit.h>
#include <chipcard2/chipcard2.h>
#include <chipcard2/sharedstuff/pininfo.h>


/** @addtogroup chipcardc_client_app Client Interface for Applications
 * @short API for Libchipcard2 clients
 *
 * Applications normally perform these steps:
 * <ol>
 *   <li>
 *     create an instance of the Libchipcard2 client (@ref LC_Client_new)
 *   </li>
 *   <li>
 *     read the client configuration file of Libchipcard2.
 *     (@ref LC_Client_ReadConfigFile)
 *   </li>
 *   <li>
 *     tell the server that we want to work with some cards
 *     (@ref LC_Client_StartWait)
 *   </li>
 *   <li>
 *     actually wait for a card to be inserted
 *     (@ref LC_Client_WaitForNextCard)
 *   </li>
 *   <li>
 *     extend the received card object to use it as special cards
 *     (like German health insurance cards, see @ref LC_KVKCard_ExtendCard)
 *   </li>
 *   <li>
 *     lock the received card so that the application can safely use
 *     it (@ref LC_Card_Open)
 *   </li>
 *   <li>
 *     optionally tell Libchipcard2 that we aren't interested in more cards
 *     (@ref LC_Client_StopWait)
 *   </li>
 *   <li>
 *     work with the card (e.g. call card specific functions like
 *     @ref LC_KVKCard_GetCardData)
 *   </li>
 *   <li>
 *     unlock the card so that other applications may use it
 *     it (@ref LC_Card_Close)
 *   </li>
 *   <li>
 *     release the card object in order to avoid memory leaks
 *     (@ref LC_Card_free)
 *   </li>
 * </ol>
 *
 * The following example illustrates this:
 *
 * @code
 * LC_CLIENT *cl;
 * LC_CARD *card=0;
 * LC_CLIENT_RESULT res;
 *
 * cl=LC_Client_new("MyApplication", "1.0", 0);
 * LC_Client_ReadConfigFile(cl, 0);
 *
 * LC_Client_StartWait(cl, 0, 0);
 * card=LC_Client_WaitForNextCard(cl, 30);
 * LC_KVKCard_ExtendCard(card);
 * LC_Card_Open(card);
 * LC_Client_StopWait(cl);
 * ...
 * [do something with the card]
 * ...
 * LC_Card_Close(card);
 * LC_Card_free(card);
 * ...
 * @endcode
 *
 */
/*@{*/
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
  LC_Client_ResultNotSupported
} LC_CLIENT_RESULT;


/**
 * Targets for commands (used by @ref LC_Card_ExecAPDU)
 */
typedef enum {
  LC_Client_CmdTargetCard=0,
  LC_Client_CmdTargetReader
} LC_CLIENT_CMDTARGET;


#include <gwenhywfar/db.h>
#include <chipcard2-client/client/card.h>
#include <chipcard2-client/client/notifications.h>
#include <chipcard2-client/mon/monitor.h>


/** @name Notify Flags/Masks
 *
 * These flags are used with @ref LC_Client_SetNotify to inform the
 * server about which events the clients wants to be informed.
 */
/*@{*/
#define LC_NOTIFY_FLAGS_DRIVER_MASK      0x0000000f
#define LC_NOTIFY_FLAGS_DRIVER_START     0x00000001
#define LC_NOTIFY_FLAGS_DRIVER_UP        0x00000002
#define LC_NOTIFY_FLAGS_DRIVER_DOWN      0x00000004
#define LC_NOTIFY_FLAGS_DRIVER_ERROR     0x00000008

#define LC_NOTIFY_FLAGS_READER_MASK      0x000000f0
#define LC_NOTIFY_FLAGS_READER_START     0x00000010
#define LC_NOTIFY_FLAGS_READER_UP        0x00000020
#define LC_NOTIFY_FLAGS_READER_DOWN      0x00000040
#define LC_NOTIFY_FLAGS_READER_ERROR     0x00000080

#define LC_NOTIFY_FLAGS_SERVICE_MASK     0x00000f00
#define LC_NOTIFY_FLAGS_SERVICE_START    0x00000100
#define LC_NOTIFY_FLAGS_SERVICE_UP       0x00000200
#define LC_NOTIFY_FLAGS_SERVICE_DOWN     0x00000400
#define LC_NOTIFY_FLAGS_SERVICE_ERROR    0x00000800

#define LC_NOTIFY_FLAGS_CARD_MASK        0x0000f000
#define LC_NOTIFY_FLAGS_CARD_INSERTED    0x00001000
#define LC_NOTIFY_FLAGS_CARD_REMOVED     0x00002000
#define LC_NOTIFY_FLAGS_CARD_RFU1        0x00004000
#define LC_NOTIFY_FLAGS_CARD_RFU2        0x00008000

#define LC_NOTIFY_FLAGS_CLIENT_MASK      0x0fff0000
#define LC_NOTIFY_FLAGS_CLIENT_UP        0x00010000
#define LC_NOTIFY_FLAGS_CLIENT_DOWN      0x00020000
#define LC_NOTIFY_FLAGS_CLIENT_STARTWAIT 0x00040000
#define LC_NOTIFY_FLAGS_CLIENT_STOPWAIT  0x00080000
#define LC_NOTIFY_FLAGS_CLIENT_TAKECARD  0x00100000
#define LC_NOTIFY_FLAGS_CLIENT_GOTCARD   0x00200000

#define LC_NOTIFY_FLAGS_CLIENT_CMDSEND   0x00400000
#define LC_NOTIFY_FLAGS_CLIENT_CMDRECV   0x00800000

#define LC_NOTIFY_FLAGS_PRIVILEGED (\
  LC_NOTIFY_FLAGS_CLIENT_CMDSEND |\
  LC_NOTIFY_FLAGS_CLIENT_CMDRECV)
/*@}*/


/** @name Prototypes for Virtual Functions
 *
 */
/*@{*/
typedef int (*LC_CLIENT_HANDLE_INREQUEST_FN)(LC_CLIENT *cl,
                                             GWEN_TYPE_UINT32 rid,
                                             GWEN_DB_NODE *dbReq);

typedef void (*LC_CLIENT_SERVER_DOWN_FN)(LC_CLIENT *cl,
                                         GWEN_TYPE_UINT32 serverId);
/*@}*/


/** @name Constructor, Destructor, Setup
 *
 */
/*@{*/

/**
 * Create an instance of the Libchipcard2 client.
 * @param programName Name of the calling program (used for improved logging
 *  on the server side)
 * @param programVersion Version of the calling program (used for improved
 * logging on the server side)
 * @param dataDir folder in which the client data (e.g. application
 * description files) is stored. You should use NULL here to make Libchipcard2
 * use the systemwide default folder.
 */
LC_CLIENT *LC_Client_new(const char *programName,
                         const char *programVersion,
                         const char *dataDir);
void LC_Client_free(LC_CLIENT *cl);

/**
 * Read the Libchipcard2 client configuration from the given GWEN_DB.
 * @param cl client object
 * @param db db which contains the configuration data
 */
int LC_Client_ReadConfig(LC_CLIENT *cl, GWEN_DB_NODE *db);

/**
 * Read the Libchipcard2 client configuration from the given configuration
 * file.
 * @param cl client object
 * @param fname path and name of the file to load. You should use NULL here
 * to make Libchipcard2 use the systemwide default configuration file.
 */
int LC_Client_ReadConfigFile(LC_CLIENT *cl, const char *fname);

/*@}*/


/** @name Functions For Inheritors
 *
 */
/*@{*/
void LC_Client_SetHandleInRequestFn(LC_CLIENT *cl,
                                    LC_CLIENT_HANDLE_INREQUEST_FN fn);
void LC_Client_SetServerDownFn(LC_CLIENT *cl,
                               LC_CLIENT_SERVER_DOWN_FN fn);

int LC_Client_SendResponse(LC_CLIENT *cl,
			   GWEN_TYPE_UINT32 rid,
			   GWEN_DB_NODE *dbCommand);

void LC_Client_RemoveInRequest(LC_CLIENT *cl, GWEN_TYPE_UINT32 rid);
/*@}*/



/** @name Informational Functions
 *
 */
/*@{*/
int LC_Client_GetShortTimeout(const LC_CLIENT *cl);
int LC_Client_GetLongTimeout(const LC_CLIENT *cl);
/*@}*/


/** @name Monitoring
 *
 */
/*@{*/
LCM_MONITOR *LC_Client_GetMonitor(const LC_CLIENT *cl);

LC_CLIENT_RESULT LC_Client_SetNotify(LC_CLIENT *cl, GWEN_TYPE_UINT32 flags);

/*@}*/


/** @name Working With Asynchronous Requests
 *
 */
/*@{*/
LC_CLIENT_RESULT LC_Client_CheckResponse(LC_CLIENT *cl, GWEN_TYPE_UINT32 rid);
LC_CLIENT_RESULT LC_Client_CheckResponse_Wait(LC_CLIENT *cl,
                                              GWEN_TYPE_UINT32 rid,
                                              int timeout);
int LC_Client_Work(LC_CLIENT *cl, int maxmsg);
LC_CLIENT_RESULT LC_Client_Work_Wait(LC_CLIENT *cl, int timeout);
int LC_Client_DeleteRequest(LC_CLIENT *cl, GWEN_TYPE_UINT32 rid);
/*@}*/



/** @name Working With Cards
 *
 */
/*@{*/

LC_CLIENT_RESULT LC_Client_StartWait(LC_CLIENT *cl,
                                     GWEN_TYPE_UINT32 rflags,
                                     GWEN_TYPE_UINT32 rmask);

GWEN_TYPE_UINT32 LC_Client_SendStartWait(LC_CLIENT *cl,
                                         GWEN_TYPE_UINT32 rflags,
                                         GWEN_TYPE_UINT32 rmask);
LC_CLIENT_RESULT LC_Client_CheckStartWait(LC_CLIENT *cl,
                                          GWEN_TYPE_UINT32 rid);


LC_CLIENT_RESULT LC_Client_StopWait(LC_CLIENT *cl);

LC_CARD *LC_Client_GetNextCard(LC_CLIENT *cl);
LC_CARD *LC_Client_PeekNextCard(LC_CLIENT *cl);
LC_CARD *LC_Client_WaitForNextCard(LC_CLIENT *cl, int timeout);
/*@}*/


/** @name Working With Services
 *
 */
/*@{*/
LC_CLIENT_RESULT LC_Client_OpenService(LC_CLIENT *cl,
                                       GWEN_TYPE_UINT32 serverId,
                                       GWEN_TYPE_UINT32 svid,
                                       GWEN_DB_NODE *dbData);

LC_CLIENT_RESULT LC_Client_CloseService(LC_CLIENT *cl,
                                        GWEN_TYPE_UINT32 serverId,
                                        GWEN_TYPE_UINT32 svid,
                                        GWEN_DB_NODE *dbData);

LC_CLIENT_RESULT LC_Client_ServiceCommand(LC_CLIENT *cl,
                                          GWEN_TYPE_UINT32 serverId,
                                          GWEN_TYPE_UINT32 svid,
                                          GWEN_DB_NODE *dbData,
                                          GWEN_DB_NODE *dbCmdResp);
/*@}*/


/** @name Working With Readers
 *
 */
/*@{*/
LC_CLIENT_RESULT LC_Client_LockReader(LC_CLIENT *cl,
                                      GWEN_TYPE_UINT32 serverId,
                                      GWEN_TYPE_UINT32 readerId,
                                      GWEN_TYPE_UINT32 *lockId);

LC_CLIENT_RESULT LC_Client_UnlockReader(LC_CLIENT *cl,
                                        GWEN_TYPE_UINT32 serverId,
                                        GWEN_TYPE_UINT32 readerId,
                                        GWEN_TYPE_UINT32 lockId);
/*@}*/


/** @name Debugging
 *
 */
/*@{*/

/**
 * Logs the given result with the loglevel "info".
 */
void LC_Card_ShowError(LC_CARD *card, LC_CLIENT_RESULT res,
                       const char *failedCommand);
/*@}*/



/*@}*/ /* defgroup */



#ifdef __cplusplus
}
#endif

#endif /* CHIPCARD_CLIENT_CLIENT_H */




