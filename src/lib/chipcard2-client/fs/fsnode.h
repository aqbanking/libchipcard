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

#define LC_FS_MODE_MASK_NODE               0x0000ffff

#define LC_FS_NODE_MODE_RIGHTS_OWNER_MASK  0x00000700
#define LC_FS_NODE_MODE_RIGHTS_OWNER_EXEC  0x00000100
#define LC_FS_NODE_MODE_RIGHTS_OWNER_WRITE 0x00000200
#define LC_FS_NODE_MODE_RIGHTS_OWNER_READ  0x00000400

#define LC_FS_NODE_MODE_RIGHTS_GROUP_MASK  0x00000070
#define LC_FS_NODE_MODE_RIGHTS_GROUP_EXEC  0x00000010
#define LC_FS_NODE_MODE_RIGHTS_GROUP_WRITE 0x00000020
#define LC_FS_NODE_MODE_RIGHTS_GROUP_READ  0x00000040

#define LC_FS_NODE_MODE_RIGHTS_OTHER_MASK  0x00000007
#define LC_FS_NODE_MODE_RIGHTS_OTHER_EXEC  0x00000001
#define LC_FS_NODE_MODE_RIGHTS_OTHER_WRITE 0x00000002
#define LC_FS_NODE_MODE_RIGHTS_OTHER_READ  0x00000004

#define LC_FS_NODE_MODE_FTYPE_MASK         0x0000f000
#define LC_FS_NODE_MODE_FTYPE_FILE         0x00001000
#define LC_FS_NODE_MODE_FTYPE_DIR          0x00002000


#define LC_FS_MODE_MASK_HANDLE             0xffff0000
#define LC_FS_HANDLE_MODE_READ             0x00010000
#define LC_FS_HANDLE_MODE_WRITE            0x00020000


typedef struct LC_FS_NODE LC_FS_NODE;
typedef struct LC_FS_NODE_HANDLE LC_FS_NODE_HANDLE;

#include <chipcard2/chipcard2.h>


GWEN_LIST_FUNCTION_LIB_DEFS(LC_FS_NODE, LC_FSNode, CHIPCARD_API)
GWEN_INHERIT_FUNCTION_LIB_DEFS(LC_FS_NODE, CHIPCARD_API)

#include <chipcard2-client/fs/fsmodule.h>


LC_FS_NODE *LC_FSNode_new(LC_FS_MODULE *fs);
void LC_FSNode_free(LC_FS_NODE *fn);
void LC_FSNode_Attach(LC_FS_NODE *fn);


GWEN_TYPE_UINT32 LC_FSNode_GetLockedById(const LC_FS_NODE *fn);
void LC_FSNode_SetLockedById(LC_FS_NODE *fn, GWEN_TYPE_UINT32 id);
GWEN_TYPE_UINT32 LC_FSNode_GetFlags(const LC_FS_NODE *fn);
void LC_FSNode_SetFlags(LC_FS_NODE *fn, GWEN_TYPE_UINT32 fl);
void LC_FSNode_AddFlags(LC_FS_NODE *fn, GWEN_TYPE_UINT32 fl);
void LC_FSNode_SubFlags(LC_FS_NODE *fn, GWEN_TYPE_UINT32 fl);

GWEN_TYPE_UINT32 LC_FSNode_GetFileMode(const LC_FS_NODE *fn);
void LC_FSNode_SetFileMode(LC_FS_NODE *fn, GWEN_TYPE_UINT32 m);

GWEN_TYPE_UINT32 LC_FSNode_GetFileSize(const LC_FS_NODE *fn);
void LC_FSNode_SetFileSize(LC_FS_NODE *fn, GWEN_TYPE_UINT32 s);

LC_FS_NODE *LC_FSNode_GetMounted(const LC_FS_NODE *fn);
void LC_FSNode_SetMounted(LC_FS_NODE *fn, LC_FS_NODE *n);
LC_FS_MODULE *LC_FSNode_GetFileSystem(const LC_FS_NODE *fn);
void LC_FSNode_SetFileSystem(LC_FS_NODE *fn, LC_FS_MODULE *fs);




LC_FS_NODE_HANDLE *LC_FSNodeHandle_new(const char *name,
                                       LC_FS_NODE *fn,
                                       GWEN_TYPE_UINT32 id);
void LC_FSNodeHandle_free(LC_FS_NODE_HANDLE *fh);
void LC_FSNodeHandle_Attach(LC_FS_NODE_HANDLE *fh);


const char *LC_FSNodeHandle_GetName(const LC_FS_NODE_HANDLE *fh);

GWEN_TYPE_UINT32 LC_FSNodeHandle_GetId(const LC_FS_NODE_HANDLE *fh);

LC_FS_NODE *LC_FSNodeHandle_GetNode(const LC_FS_NODE_HANDLE *fh);

GWEN_TYPE_UINT32 LC_FSNodeHandle_GetFlags(const LC_FS_NODE_HANDLE *fh);
void LC_FSNodeHandle_SetFlags(LC_FS_NODE_HANDLE *fh, GWEN_TYPE_UINT32 fl);
void LC_FSNodeHandle_AddFlags(LC_FS_NODE_HANDLE *fh, GWEN_TYPE_UINT32 fl);
void LC_FSNodeHandle_SubFlags(LC_FS_NODE_HANDLE *fh, GWEN_TYPE_UINT32 fl);

GWEN_TYPE_UINT32 LC_FSNodeHandle_GetFilePointer(const LC_FS_NODE_HANDLE *fh);
void LC_FSNodeHandle_SetFilePointer(LC_FS_NODE_HANDLE *fh,
                                    GWEN_TYPE_UINT32 fpos);

int LC_FSNodeHandle_GetFirstEntry(LC_FS_NODE_HANDLE *fh,
                                  GWEN_BUFFER *ebuf);
int LC_FSNodeHandle_GetNextEntry(LC_FS_NODE_HANDLE *fh,
                                 GWEN_BUFFER *ebuf);
void LC_FSNodeHandle_AddEntry(LC_FS_NODE_HANDLE *fh,
                              const char *name);

#endif /* LC_FS_NODE_H */


