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


#ifndef LC_FS_P_H
#define LC_FS_P_H


#include <gwenhywfar/misc.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/types.h>

#include "fsmodule_l.h"


struct LC_FS_STAT {
  GWEN_TYPE_UINT32 flags;
  GWEN_TYPE_UINT32 length;
};


struct LC_FS_MODULE {
  GWEN_INHERIT_ELEMENT(LC_FS_MODULE);
  GWEN_LIST_ELEMENT(LC_FS_MODULE);

  LC_FS_MODULE_MOUNT_FN mountFn;
  LC_FS_MODULE_UNMOUNT_FN unmountFn;

  LC_FS_MODULE_OPENDIR_FN openDirFn;
  LC_FS_MODULE_MKDIR_FN mkDirFn;
  LC_FS_MODULE_READDIR_FN readDirFn;
  LC_FS_MODULE_CLOSEDIR_FN closeDirFn;

  LC_FS_MODULE_OPENFILE_FN openFileFn;
  LC_FS_MODULE_CREATEFILE_FN createFileFn;
  LC_FS_MODULE_CLOSEFILE_FN closeFileFn;
  LC_FS_MODULE_READFILE_FN readFileFn;
  LC_FS_MODULE_WRITEFILE_FN writeFileFn;
  LC_FS_MODULE_STAT_FN statFn;

  GWEN_TYPE_UINT32 flags;
  GWEN_TYPE_UINT32 activeNodes;
  GWEN_TYPE_UINT32 usage;

};


#endif /* LC_FS_MODULE_P_H */

