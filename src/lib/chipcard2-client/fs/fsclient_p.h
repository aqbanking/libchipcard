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


#ifndef LC_FS_CLIENT_P_H
#define LC_FS_CLIENT_P_H


#include "fsclient_l.h"

struct LC_FS_CLIENT {
  GWEN_TYPE_UINT32 id;
  LC_FS_NODE_HANDLE_LIST2 *handles;
  LC_FS_NODE *nWorkingDir;
  char *cWorkingDir;
};


#endif /* LC_FS_CLIENT_P_H */
