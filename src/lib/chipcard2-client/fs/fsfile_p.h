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


#ifndef LC_FSFILE_P_H
#define LC_FSFILE_P_H


#include <chipcard2-client/fs/fsfile.h>
#include <stdio.h>


struct LC_FSFILE_NODE {
  LC_FS_NODE *parent;
  LC_FS_NODE_LIST *children;
  char *name;
  int sampled;
};

void LC_FSFileNode_FreeData(void *bp, void *p);

LC_FS_NODE_LIST *LC_FSFileNode_GetChildren(const LC_FS_NODE *n);
void LC_FSFileNode_AddChild(LC_FS_NODE *n, LC_FS_NODE *nchild);

LC_FS_NODE *LC_FSFileNode_GetParent(const LC_FS_NODE *n);
void LC_FSFileNode_SetParent(LC_FS_NODE *n, LC_FS_NODE *p);

const char *LC_FSFileNode_GetName(const LC_FS_NODE *n);
void LC_FSFileNode_SetName(LC_FS_NODE *n, const char *name);

int LC_FSFileNode_GetSampled(const LC_FS_NODE *n);
void LC_FSFileNode_SetSampled(LC_FS_NODE *n, int b);


void LC_FSFileNode_Dump(LC_FS_NODE *node, FILE *f, int indent);



struct LC_FSFILE_MODULE {
  char *path;
  GWEN_TYPE_UINT32 mountFlags;
};

void LC_FSFileModule_FreeData(void *bp, void *p);


LC_FS_NODE *LC_FSFileModule__FindNode(LC_FS_MODULE *fs,
                                     LC_FS_NODE *node,
                                     const char *name);


int LC_FSFileModule_Mount(LC_FS_MODULE *fs,
                          LC_FS_NODE **nPtr);

int LC_FSFileModule_Unmount(LC_FS_MODULE *fs, LC_FS_NODE *node);


int LC_FSFileModule_OpenDir(LC_FS_MODULE *fs,
                           LC_FS_NODE *node,
                           const char *name,
                           LC_FS_NODE **nPtr);

int LC_FSFileModule_MkDir(LC_FS_MODULE *fs,
                         LC_FS_NODE *node,
                         const char *name,
                         GWEN_TYPE_UINT32 flags,
                         LC_FS_NODE **nPtr);


int LC_FSFileModule_ReadDir(LC_FS_MODULE *fs,
                           LC_FS_NODE *node,
                           GWEN_STRINGLIST2 *sl);

int LC_FSFileModule_CloseDir(LC_FS_MODULE *fs,
                            LC_FS_NODE *node);


int LC_FSFileModule_OpenFile(LC_FS_MODULE *fs,
                            LC_FS_NODE *node,
                            const char *name,
                            LC_FS_NODE **nPtr);

int LC_FSFileModule_CreateFile(LC_FS_MODULE *fs,
                              LC_FS_NODE *node,
                              const char *name,
                              GWEN_TYPE_UINT32 flags,
                              LC_FS_NODE **nPtr);

int LC_FSFileModule_CloseFile(LC_FS_MODULE *fs,
                             LC_FS_NODE *node);


int LC_FSFileModule_ReadFile(LC_FS_MODULE *fs,
                             LC_FS_NODE *node,
                             GWEN_TYPE_UINT32 mode,
                             GWEN_TYPE_UINT32 offset,
                             GWEN_TYPE_UINT32 len,
                             GWEN_BUFFER *buf);

int LC_FSFileModule_WriteFile(LC_FS_MODULE *fs,
                              LC_FS_NODE *node,
                              GWEN_TYPE_UINT32 mode,
                              GWEN_TYPE_UINT32 offset,
                              GWEN_BUFFER *buf);

int LC_FSFileModule_Lookup(LC_FS_MODULE *fs,
                          LC_FS_NODE *node,
                          const char *name,
                          LC_FS_NODE **nPtr);


int LC_FSFileModule_Dump(LC_FS_MODULE *fs,
                        LC_FS_NODE *node,
                        FILE *f,
                        int indent);


int LC_FSFileModule__Dir2Node2(LC_FS_MODULE *fs,
                               LC_FS_NODE *node,
                               const char *path);

int LC_FSFileModule__GetNodePath(LC_FS_MODULE *fs,
                                 LC_FS_NODE *node,
                                 GWEN_BUFFER *pbuf);

int LC_FSFileModule__FileModeToSys(GWEN_TYPE_UINT32 fm);


#endif

