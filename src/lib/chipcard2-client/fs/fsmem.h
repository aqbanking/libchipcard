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


#ifndef LC_FSMEM_H
#define LC_FSMEM_H


#include <chipcard2-client/fs/fsmodule.h>
#include <chipcard2-client/fs/fsnode.h>



typedef struct LC_FSMEM_NODE LC_FSMEM_NODE;
typedef struct LC_FSMEM_MODULE LC_FSMEM_MODULE;

LC_FS_NODE *LC_FSMemNode_new(LC_FS_MODULE *fs,
                             const char *name);


LC_FS_MODULE *LC_FSMemModule_new();





#endif

