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


#ifndef CHIPCARD_SERVER_CARDSERVER_L_H
#define CHIPCARD_SERVER_CARDSERVER_L_H

#include <chipcard2-server/server/cardserver.h>



LC_CARD *LC_CardServer_FindActiveCard(const LC_CARDSERVER *cs,
                                      GWEN_TYPE_UINT32 cid);

GWEN_TYPE_UINT32 LC_CardServer_CheckConnForDriver(LC_CARDSERVER *cs,
                                                  LC_DRIVER *d);


GWEN_TYPE_UINT32 LC_CardServer_GetNotificationMask(const char *ntype,
                                                   const char *ncode);

/**
 *
 */
int LC_CardServer__SendNotification(LC_CARDSERVER *cs,
                                    const LC_CLIENT *cl,
                                    const char *ntype,
                                    const char *ncode,
                                    GWEN_DB_NODE *dbData);

int LC_CardServer_SendNotification(LC_CARDSERVER *cs,
                                   LC_CLIENT *cl,
                                   const char *ntype,
                                   const char *ncode,
                                   GWEN_DB_NODE *dbData);

int LC_CardServer_SendDriverNotification(LC_CARDSERVER *cs,
                                         LC_CLIENT *cl,
                                         const char *ncode,
                                         const LC_DRIVER *d,
                                         const char *info);

int LC_CardServer_SendReaderNotification(LC_CARDSERVER *cs,
                                         LC_CLIENT *cl,
                                         const char *ncode,
                                         const LC_READER *r,
                                         const char *info);

void LC_CardServer_DumpState(LC_CARDSERVER *cs);


#endif /* CHIPCARD_SERVER_CARDSERVER_L_H */




