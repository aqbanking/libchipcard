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


#ifndef CHIPCARD_CLIENT_MSGENGINE_P_H
#define CHIPCARD_CLIENT_MSGENGINE_P_H


#include "msgengine_l.h"

#define LC_KVK_UMLAUT_AE 0x5b
#define LC_KVK_UMLAUT_OE 0x5c
#define LC_KVK_UMLAUT_UE 0x5d
#define LC_KVK_UMLAUT_ae 0x7b
#define LC_KVK_UMLAUT_oe 0x7c
#define LC_KVK_UMLAUT_ue 0x7d
#define LC_KVK_UMLAUT_ss 0x7e


typedef struct LC_MSGENGINE LC_MSGENGINE;

struct LC_MSGENGINE {
};



void LC_MsgEngine_FreeData(void *bp, void *p);

int LC_MsgEngine_TypeRead(GWEN_MSGENGINE *e,
                          GWEN_BUFFER *msgbuf,
                          GWEN_XMLNODE *node,
                          GWEN_BUFFER *vbuf,
                          char escapeChar,
                          const char *delimiters);

int LC_MsgEngine_TypeWrite(GWEN_MSGENGINE *e,
                           GWEN_BUFFER *gbuf,
                           GWEN_BUFFER *data,
                           GWEN_XMLNODE *node);

GWEN_DB_VALUETYPE LC_MsgEngine_TypeCheck(GWEN_MSGENGINE *e,
                                         const char *tname);

const char *LC_MsgEngine_GetCharValue(GWEN_MSGENGINE *e,
                                      const char *name,
                                      const char *defValue);

int LC_MsgEngine_GetIntValue(GWEN_MSGENGINE *e,
                             const char *name,
                             int defValue);


int LC_MsgEngine_BinTypeRead(GWEN_MSGENGINE *e,
                             GWEN_XMLNODE *node,
                             GWEN_DB_NODE *gr,
                             GWEN_BUFFER *vbuf);

int LC_MsgEngine_BinTypeWrite(GWEN_MSGENGINE *e,
                              GWEN_XMLNODE *node,
                              GWEN_DB_NODE *gr,
                              GWEN_BUFFER *dbuf);


GWEN_TYPE_UINT32 LC_MsgEngine__FromBCD(GWEN_TYPE_UINT32 value);
GWEN_TYPE_UINT32 LC_MsgEngine__ToBCD(GWEN_TYPE_UINT32 value);



#endif /* CHIPCARD_CLIENT_MSGENGINE_P_H */


