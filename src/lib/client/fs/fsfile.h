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


#ifndef LC_FSFILE_H
#define LC_FSFILE_H


#include <chipcard2-client/fs/fsmodule.h>
#include <chipcard2-client/fs/fsnode.h>



typedef struct LC_FSFILE_NODE LC_FSFILE_NODE;
typedef struct LC_FSFILE_MODULE LC_FSFILE_MODULE;

LC_FS_NODE *LC_FSFileNode_new(LC_FS_MODULE *fs,
                              const char *name);


LC_FS_MODULE *LC_FSFileModule_new(const char *path);


/* DEBUG */
int LC_FSFileModule__Dir2Node2(LC_FS_MODULE *fs,
                               LC_FS_NODE *node,
                               const char *path);



#endif

