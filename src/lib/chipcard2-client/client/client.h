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


typedef struct LC_CLIENT LC_CLIENT;

GWEN_INHERIT_FUNCTION_DEFS(LC_CLIENT)


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
  LC_Client_ResultNoData
} LC_CLIENT_RESULT;


typedef enum {
  LC_Client_CmdTargetCard=0,
  LC_Client_CmdTargetReader
} LC_CLIENT_CMDTARGET;


#include <gwenhywfar/db.h>
#include <chipcard2-client/client/card.h>
#include <chipcard2-client/client/notifications.h>
#include <chipcard2-client/mon/monitor.h>


#define LC_CLIENT_CBID_IO_WAITRSP "LC_CLIENT_CBID_WAITRSP"
#define LC_CLIENT_CBID_IO_WAITCARD "LC_CLIENT_CBID_WAITCARD"


/** @name Notify Flags/Masks
 *
 *
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


/** @name Notify Types/Codes
 *
 *
 */
/*@{*/
#define LC_NOTIFY_TYPE_DRIVER           "driver"
#define LC_NOTIFY_CODE_DRIVER_START     "start"
#define LC_NOTIFY_CODE_DRIVER_UP        "up"
#define LC_NOTIFY_CODE_DRIVER_DOWN      "down"
#define LC_NOTIFY_CODE_DRIVER_ERROR     "error"
#define LC_NOTIFY_CODE_DRIVER_REMOVED   "removed"

#define LC_NOTIFY_TYPE_READER           "reader"
#define LC_NOTIFY_CODE_READER_START     "start"
#define LC_NOTIFY_CODE_READER_UP        "up"
#define LC_NOTIFY_CODE_READER_DOWN      "down"
#define LC_NOTIFY_CODE_READER_ERROR     "error"
#define LC_NOTIFY_CODE_READER_REMOVED   "removed"

#define LC_NOTIFY_TYPE_SERVICE          "service"
#define LC_NOTIFY_CODE_SERVICE_START    "start"
#define LC_NOTIFY_CODE_SERVICE_UP       "up"
#define LC_NOTIFY_CODE_SERVICE_DOWN     "down"
#define LC_NOTIFY_CODE_SERVICE_ERROR    "error"

#define LC_NOTIFY_TYPE_CARD             "card"
#define LC_NOTIFY_CODE_CARD_INSERTED    "inserted"
#define LC_NOTIFY_CODE_CARD_REMOVED     "removed"
#define LC_NOTIFY_CODE_CARD_RFU1        "rfu1"
#define LC_NOTIFY_CODE_CARD_RFU2        "rfu2"

#define LC_NOTIFY_TYPE_CLIENT           "client"
#define LC_NOTIFY_CODE_CLIENT_UP        "up"
#define LC_NOTIFY_CODE_CLIENT_DOWN      "down"
#define LC_NOTIFY_CODE_CLIENT_STARTWAIT "startwait"
#define LC_NOTIFY_CODE_CLIENT_STOPWAIT  "stopwait"
#define LC_NOTIFY_CODE_CLIENT_TAKECARD  "takecard"
#define LC_NOTIFY_CODE_CLIENT_GOTCARD   "gotcard"

#define LC_NOTIFY_CODE_CLIENT_CMDSEND   "cmdsend"
#define LC_NOTIFY_CODE_CLIENT_CMDRECV   "cmdrecv"
/*@}*/



typedef int (*LC_CLIENT_HANDLE_INREQUEST_FN)(LC_CLIENT *cl,
                                             GWEN_TYPE_UINT32 rid,
                                             GWEN_DB_NODE *dbReq);

typedef void (*LC_CLIENT_SERVER_DOWN_FN)(LC_CLIENT *cl,
                                         GWEN_TYPE_UINT32 serverId);


/** @name Constructor, Destructor, Setup
 *
 */
/*@{*/
LC_CLIENT *LC_Client_new(const char *programName,
                         const char *programVersion,
                         const char *dataDir);
void LC_Client_free(LC_CLIENT *cl);

int LC_Client_ReadConfig(LC_CLIENT *cl, GWEN_DB_NODE *db);

int LC_Client_ReadConfigFile(LC_CLIENT *cl,
                             const char *fname);

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

GWEN_DB_NODE *LC_Client_GetNextResponse(LC_CLIENT *cl,
                                        GWEN_TYPE_UINT32 rqid);
GWEN_DB_NODE *LC_Client_WaitForNextResponse(LC_CLIENT *cl,
                                            GWEN_TYPE_UINT32 rqid,
                                            int timeout);

int LC_Client_CheckForError(GWEN_DB_NODE *db);

GWEN_TYPE_UINT32 LC_Client_SendRequest(LC_CLIENT *cl,
                                       LC_CARD *card,
				       GWEN_TYPE_UINT32 serverId,
				       GWEN_DB_NODE *dbReq);

/*@}*/



/** @name Informational Functions
 *
 */
/*@{*/
int LC_Client_GetShortTimeout(const LC_CLIENT *cl);
int LC_Client_GetLongTimeout(const LC_CLIENT *cl);

LCM_MONITOR *LC_Client_GetMonitor(const LC_CLIENT *cl);

/*@}*/



/** @name Work And Request Handling
 *
 */
/*@{*/

LC_CLIENT_RESULT
  LC_Client_CheckResponse(LC_CLIENT *cl, GWEN_TYPE_UINT32 rid);
LC_CLIENT_RESULT
  LC_Client_CheckResponse_Wait(LC_CLIENT *cl,
                               GWEN_TYPE_UINT32 rid,
                               int timeout);

int LC_Client_DeleteRequest(LC_CLIENT *cl, GWEN_TYPE_UINT32 rid);

int LC_Client_Work(LC_CLIENT *cl, int maxmsg);
LC_CLIENT_RESULT LC_Client_Work_Wait(LC_CLIENT *cl, int timeout);

/*@}*/


/** @name Commands: Asynchronous API
 *
 * <p>
 * This group contains commands to be send to a chipcard daemon.
 * For every request's send-function (LC_Client_SendXYZ) there is a
 * matching check-function (LC_Client_CheckXYZ) which checks for a
 * response to the request.
 * </p>
 * <p>
 * Every request gets an unique id assigned to it which MUST then
 * be used to refer to it.
 * </p>
 */
/*@{*/

GWEN_TYPE_UINT32 LC_Client_SendStartWait(LC_CLIENT *cl,
                                         GWEN_TYPE_UINT32 rflags,
                                         GWEN_TYPE_UINT32 rmask);
LC_CLIENT_RESULT
  LC_Client_CheckStartWait(LC_CLIENT *cl,
                           GWEN_TYPE_UINT32 rid);



GWEN_TYPE_UINT32 LC_Client_SendStopWait(LC_CLIENT *cl);
LC_CLIENT_RESULT
  LC_Client_CheckStopWait(LC_CLIENT *cl,
                          GWEN_TYPE_UINT32 rid);


LC_CARD *LC_Client_GetNextCard(LC_CLIENT *cl);
LC_CARD *LC_Client_PeekNextCard(LC_CLIENT *cl);


GWEN_TYPE_UINT32 LC_Client_SendTakeCard(LC_CLIENT *cl, LC_CARD *cd);
LC_CLIENT_RESULT LC_Client_CheckTakeCard(LC_CLIENT *cl,
                                         GWEN_TYPE_UINT32 rid);

GWEN_TYPE_UINT32 LC_Client_SendReleaseCard(LC_CLIENT *cl, LC_CARD *cd);
LC_CLIENT_RESULT LC_Client_CheckReleaseCard(LC_CLIENT *cl,
                                            GWEN_TYPE_UINT32 rid);

GWEN_TYPE_UINT32 LC_Client_SendCommandCard(LC_CLIENT *cl,
                                           LC_CARD *cd,
                                           const char *apdu,
                                           unsigned int len,
                                           LC_CLIENT_CMDTARGET t);
LC_CLIENT_RESULT LC_Client_CheckCommandCard(LC_CLIENT *cl,
                                            GWEN_TYPE_UINT32 rid,
                                            GWEN_BUFFER *data);


GWEN_TYPE_UINT32 LC_Client_SendSelectCardApp(LC_CLIENT *cl,
                                             LC_CARD *cd,
                                             const char *cardName,
                                             const char *appName);
LC_CLIENT_RESULT LC_Client_CheckSelectCardApp(LC_CLIENT *cl,
                                              GWEN_TYPE_UINT32 rid);


GWEN_TYPE_UINT32 LC_Client_SendExecCommand(LC_CLIENT *cl,
                                           LC_CARD *cd,
                                           GWEN_DB_NODE *dbCmd);
LC_CLIENT_RESULT LC_Client_CheckExecCommand(LC_CLIENT *cl,
                                            GWEN_TYPE_UINT32 rid,
                                            GWEN_DB_NODE *dbRsp);

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


/*@}*/


/** @name Commands: Synchronous API
 *
 * <p>
 * Functions in this group wait until the server finished the execution
 * of a command.
 * </p>
 * <p>
 * This API is simpler than the asynchronous API. Please note that special
 * card commands are described in @ref LC_CARD and classes derived thereof.
 * </p>
 */
/*@{*/

LC_CARD *LC_Client_WaitForNextCard(LC_CLIENT *cl, int timeout);

LC_CLIENT_RESULT LC_Client_StartWait(LC_CLIENT *cl,
                                     GWEN_TYPE_UINT32 rflags,
                                     GWEN_TYPE_UINT32 rmask);
LC_CLIENT_RESULT LC_Client_StopWait(LC_CLIENT *cl);

LC_CLIENT_RESULT LC_Client_SetNotify(LC_CLIENT *cl, GWEN_TYPE_UINT32 flags);

LC_CLIENT_RESULT LC_Client_GetDriverVar(LC_CLIENT *cl,
                                        LC_CARD *card,
                                        const char *vname,
                                        GWEN_BUFFER *vbuf);


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




#ifdef __cplusplus
}
#endif

#endif /* CHIPCARD_CLIENT_CLIENT_H */




