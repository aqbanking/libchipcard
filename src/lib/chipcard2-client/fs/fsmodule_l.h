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


#ifndef LC_FS_MODULE_L_H
#define LC_FS_MODULE_L_H


#include <chipcard2-client/fs/fsmodule.h>
#include <stdio.h>


GWEN_LIST_FUNCTION_DEFS(LC_FS_MODULE, LC_FSModule)


int LC_FSModule_Mount(LC_FS_MODULE *fs,
                      LC_FS_NODE **nPtr);

int LC_FSModule_Unmount(LC_FS_MODULE *fs,
                        LC_FS_NODE *node);


int LC_FSModule_OpenDir(LC_FS_MODULE *fs,
                        LC_FS_NODE *node,
                        const char *name,
                        LC_FS_NODE **nPtr);

int LC_FSModule_MkDir(LC_FS_MODULE *fs,
                      LC_FS_NODE *node,
                      const char *name,
                      GWEN_TYPE_UINT32 mode,
                      LC_FS_NODE **nPtr);


int LC_FSModule_ReadDir(LC_FS_MODULE *fs,
                        LC_FS_NODE *node,
                        GWEN_STRINGLIST2 *sl);

int LC_FSModule_CloseDir(LC_FS_MODULE *fs,
                         LC_FS_NODE *node);


int LC_FSModule_OpenFile(LC_FS_MODULE *fs,
                         LC_FS_NODE *node,
                         const char *name,
                         LC_FS_NODE **nPtr);

int LC_FSModule_CreateFile(LC_FS_MODULE *fs,
                           LC_FS_NODE *node,
                           const char *name,
                           GWEN_TYPE_UINT32 flags,
                           LC_FS_NODE **nPtr);

int LC_FSModule_CloseFile(LC_FS_MODULE *fs,
                          LC_FS_NODE *node);


int LC_FSModule_ReadFile(LC_FS_MODULE *fs,
                         LC_FS_NODE *node,
                         GWEN_TYPE_UINT32 mode,
                         GWEN_TYPE_UINT32 offset,
                         GWEN_TYPE_UINT32 len,
                         GWEN_BUFFER *buf);

int LC_FSModule_WriteFile(LC_FS_MODULE *fs,
                          LC_FS_NODE *node,
                          GWEN_TYPE_UINT32 mode,
                          GWEN_TYPE_UINT32 offset,
                          GWEN_BUFFER *buf);

int LC_FSModule_Lookup(LC_FS_MODULE *fs,
                       LC_FS_NODE *node,
                       const char *name,
                       LC_FS_NODE **nPtr);



#endif /* LC_FS_MODULE_L_H */
