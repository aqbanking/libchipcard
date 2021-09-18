/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2021 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CLIENT_CLIENT_P_H
#define CHIPCARD_CLIENT_CLIENT_P_H

#include "client_l.h"

#include <gwenhywfar/msgengine.h>

#include <winscard.h>
#ifndef OS_WIN32
# include <wintypes.h>
#endif


#define LCC_PM_LIBNAME    "libchipcard"
#define LCC_PM_SYSCONFDIR "sysconfdir"
#define LCC_PM_DATADIR    "datadir"


#define MAX_READERS 32



struct LC_CLIENT {
  GWEN_INHERIT_ELEMENT(LC_CLIENT)
  char *programName;
  char *programVersion;

  GWEN_MSGENGINE *msgEngine;
  GWEN_XMLNODE *cardNodes;
  GWEN_XMLNODE *appNodes;

  SCARDCONTEXT scardContext;

  int pnpAvailable;
  SCARD_READERSTATE readerStates[MAX_READERS];
  int readerCount;
  int lastUsedReader;
  LPSTR readerList; /* multistring containing result of SCardListReaders */
};



#endif /* CHIPCARD_CLIENT_CLIENT_P_H */



