/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: cs_init.c 282 2006-09-21 16:52:04Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "server_p.h"
#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/directory.h>

#include <gwenhywfar/ipc.h>
#include <gwenhywfar/pathmanager.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "cs_callbacks.c"
#include "cs_init.c"
#include "cs_tools.c"
#include "cs_work.c"


