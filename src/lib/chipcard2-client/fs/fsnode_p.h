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


#ifndef LC_FS_NODE_P_H
#define LC_FS_NODE_P_H


#include "fsnode_l.h"



struct LC_FS_NODE {
  GWEN_INHERIT_ELEMENT(LC_FS_NODE);
  GWEN_LIST_ELEMENT(LC_FS_NODE);
  GWEN_TYPE_UINT32 lockedById;
  GWEN_TYPE_UINT32 flags;
  GWEN_TYPE_UINT32 usageCounter;
  LC_FS_NODE *mounted;

  GWEN_TYPE_UINT32 fileMode;
  GWEN_TYPE_UINT32 fileSize;
  time_t ctime;
  time_t atime;
  time_t mtime;
  GWEN_TYPE_UINT32 linkCount;

  LC_FS_MODULE *fileSystem;
};




struct LC_FS_NODE_HANDLE {
  GWEN_LIST_ELEMENT(LC_FS_NODE_HANDLE);
  GWEN_TYPE_UINT32 usageCounter;
  char *name;
  GWEN_TYPE_UINT32 id;
  LC_FS_NODE *node;
  GWEN_TYPE_UINT32 flags;
  GWEN_TYPE_UINT32 fpointer;
  GWEN_STRINGLIST2 *entryList;
  GWEN_STRINGLIST2_ITERATOR *entryIterator;
};



#endif /* LC_FS_NODE_P_H */
