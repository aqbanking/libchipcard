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


#ifndef LC_RFS_P_H
#define LC_RFS_P_H

#include <chipcard2-client/fs/rfs.h>
#include <gwenhywfar/buffer.h>

#include "fsclient_l.h"



struct LC_RFS {
  GWEN_INHERIT_ELEMENT(LC_RFS)
  LC_RFS_EXCHANGE_FN exchangeFn;
};




#endif /* LC_FS_P_H */
