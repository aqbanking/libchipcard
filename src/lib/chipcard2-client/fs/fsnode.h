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


#ifndef LC_FS_NODE_H
#define LC_FS_NODE_H

#include <gwenhywfar/misc.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/stringlist2.h>


typedef struct LC_FS_NODE LC_FS_NODE;
typedef struct LC_FS_NODE_HANDLE LC_FS_NODE_HANDLE;

#include <chipcard2-client/chipcard2.h>


GWEN_LIST_FUNCTION_LIB_DEFS(LC_FS_NODE, LC_FSNode, LC_CLIENT_API)
GWEN_INHERIT_FUNCTION_LIB_DEFS(LC_FS_NODE, LC_CLIENT_API)

#include <chipcard2-client/fs/fsmodule.h>





#endif /* LC_FS_NODE_H */


