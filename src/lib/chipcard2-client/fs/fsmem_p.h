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


#ifndef LC_FSMEM_P_H
#define LC_FSMEM_P_H


#include <chipcard2-client/fs/fsmodule.h>
#include <chipcard2-client/fs/fsnode.h>



typedef struct LC_FSMEM_NODE LC_FSMEM_NODE;
struct LC_FSMEM_NODE {
  LC_FS_NODE *parent;
  LC_FS_NODE_LIST *children;
  char *name;
  GWEN_BUFFER *data;
};

LC_FS_NODE *LC_FSMemNode_new(LC_FS_MODULE *fs);
void LC_FSMemNode_FreeData(void *bp, void *p);

LC_FS_NODE_LIST *LC_FSMemNode_GetChildren(const LC_FS_NODE *n);
void LC_FSMemNode_AddChild(LC_FS_NODE *n, LC_FS_NODE *nchild);

LC_FS_NODE *LC_FSMemNode_GetParent(const LC_FS_NODE *n);
void LC_FSMemNode_SetParent(LC_FS_NODE *n, LC_FS_NODE *p);

const char *LC_FSMemNode_GetName(const LC_FS_NODE *n);
void LC_FSMemNode_SetName(LC_FS_NODE *n, const char *name);

GWEN_BUFFER *LC_FSMemNode_GetDataBuffer(const LC_FS_NODE *n);




typedef struct LC_FSMEM_MODULE LC_FSMEM_MODULE;
struct LC_FSMEM_MODULE {
  LC_FS_NODE *root;
};

LC_FS_MODULE *LC_FSMemModule_new();
void LC_FSMemModule_FreeData(void *bp, void *p);


LC_FS_NODE *LC_FSMemModule__FindNode(LC_FS_MODULE *fs,
                                     LC_FS_NODE *node,
                                     const char *name);


int LC_FSMemModule_Mount(LC_FS_MODULE *fs);

int LC_FSMemModule_Unmount(LC_FS_MODULE *fs);


int LC_FSMemModule_OpenDir(LC_FS_MODULE *fs,
                           LC_FS_NODE *node,
                           const char *name,
                           LC_FS_NODE **nPtr);

int LC_FSMemModule_MkDir(LC_FS_MODULE *fs,
                         LC_FS_NODE *node,
                         const char *name,
                         GWEN_TYPE_UINT32 flags,
                         LC_FS_NODE **nPtr);


int LC_FSMemModule_ReadDir(LC_FS_MODULE *fs,
                           LC_FS_NODE *node,
                           GWEN_STRINGLIST2 *sl);

int LC_FSMemModule_CloseDir(LC_FS_MODULE *fs,
                            LC_FS_NODE *node);


int LC_FSMemModule_OpenFile(LC_FS_MODULE *fs,
                            LC_FS_NODE *node,
                            const char *name,
                            LC_FS_NODE **nPtr);

int LC_FSMemModule_CreateFile(LC_FS_MODULE *fs,
                              LC_FS_NODE *node,
                              const char *name,
                              GWEN_TYPE_UINT32 flags,
                              LC_FS_NODE **nPtr);

int LC_FSMemModule_CloseFile(LC_FS_MODULE *fs,
                             LC_FS_NODE *node);


int LC_FSMemModule_ReadFile(LC_FS_MODULE *fs,
                            LC_FS_NODE *node,
                            GWEN_TYPE_UINT32 offset,
                            GWEN_TYPE_UINT32 len,
                            GWEN_BUFFER *buf);

int LC_FSMemModule_WriteFile(LC_FS_MODULE *fs,
                             LC_FS_NODE *node,
                             GWEN_TYPE_UINT32 offset,
                             GWEN_BUFFER *buf);

int LC_FSMemModule_Lookup(LC_FS_MODULE *fs,
                          LC_FS_NODE *node,
                          const char *name,
                          LC_FS_NODE **nPtr);





#endif

