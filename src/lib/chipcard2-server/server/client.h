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



#ifndef CHIPCARD_SERVER_CLIENT_H
#define CHIPCARD_SERVER_CLIENT_H

typedef struct LC_CLIENT  LC_CLIENT;


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

#define LC_NOTIFY_TYPE_READER           "reader"
#define LC_NOTIFY_CODE_READER_START     "start"
#define LC_NOTIFY_CODE_READER_UP        "up"
#define LC_NOTIFY_CODE_READER_DOWN      "down"
#define LC_NOTIFY_CODE_READER_ERROR     "error"

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




#include <gwenhywfar/misc.h>


GWEN_LIST_FUNCTION_DEFS(LC_CLIENT, LC_Client);



LC_CLIENT *LC_Client_new(GWEN_TYPE_UINT32 id);
void LC_Client_free(LC_CLIENT *cl);

GWEN_TYPE_UINT32 LC_Client_GetClientId(const LC_CLIENT *cl);

int LC_Client_HasReader(const LC_CLIENT *cl, GWEN_TYPE_UINT32 id);
int LC_Client_AddReader(const LC_CLIENT *cl, GWEN_TYPE_UINT32 id);
int LC_Client_DelReader(const LC_CLIENT *cl, GWEN_TYPE_UINT32 id);

int LC_Client_HasCard(const LC_CLIENT *cl, GWEN_TYPE_UINT32 id);
int LC_Client_AddCard(LC_CLIENT *cl, GWEN_TYPE_UINT32 id);
int LC_Client_DelCard(LC_CLIENT *cl, GWEN_TYPE_UINT32 id);
void LC_Client_DelAllCards(LC_CLIENT *cl);

int LC_Client_HasService(const LC_CLIENT *cl, GWEN_TYPE_UINT32 id);
int LC_Client_AddService(LC_CLIENT *cl, GWEN_TYPE_UINT32 id);
int LC_Client_DelService(LC_CLIENT *cl, GWEN_TYPE_UINT32 id);


GWEN_TYPE_UINT32 LC_Client_GetWaitRequestCount(const LC_CLIENT *cl);
void LC_Client_AddWaitRequestCount(LC_CLIENT *cl);
void LC_Client_SubWaitRequestCount(LC_CLIENT *cl);

GWEN_TYPE_UINT32 LC_Client_GetLastWaitRequestId(const LC_CLIENT *cl);
void LC_Client_SetLastWaitRequestId(LC_CLIENT *cl, GWEN_TYPE_UINT32 id);

GWEN_TYPE_UINT32 LC_Client_GetWaitReaderFlags(const LC_CLIENT *cl);
GWEN_TYPE_UINT32 LC_Client_GetWaitReaderMask(const LC_CLIENT *cl);
void LC_Client_AddWaitReaderState(LC_CLIENT *cl,
                                  GWEN_TYPE_UINT32 flags,
                                  GWEN_TYPE_UINT32 mask);
void LC_Client_ResetWaitReaderState(LC_CLIENT *cl);

const char *LC_Client_GetApplicationName(const LC_CLIENT *cl);
void LC_Client_SetApplicationName(LC_CLIENT *cl, const char *s);

const char *LC_Client_GetUserName(const LC_CLIENT *cl);
void LC_Client_SetUserName(LC_CLIENT *cl, const char *s);


GWEN_TYPE_UINT32 LC_Client_GetNotifyFlags(const LC_CLIENT *cl);
void LC_Client_SetNotifyFlags(LC_CLIENT *cl,
                              GWEN_TYPE_UINT32 flags);
void LC_Client_AddNotifyFlags(LC_CLIENT *cl,
                              GWEN_TYPE_UINT32 flags);
void LC_Client_DelNotifyFlags(LC_CLIENT *cl,
                              GWEN_TYPE_UINT32 flags);

GWEN_TYPE_UINT32 LC_Client_GetNotifyMask(const LC_CLIENT *cl);
void LC_Client_SetNotifyMask(LC_CLIENT *cl,
                              GWEN_TYPE_UINT32 mask);
void LC_Client_AddNotifyMask(LC_CLIENT *cl,
                              GWEN_TYPE_UINT32 mask);
void LC_Client_DelNotifyMask(LC_CLIENT *cl,
                              GWEN_TYPE_UINT32 mask);



#endif /* CHIPCARD_SERVER_CLIENT_H */





