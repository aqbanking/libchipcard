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


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include <gwenhywfar/debug.h>
#include <gwenhywfar/gwenhywfar.h>

#include <unistd.h>



int main(int argc, char **argv) {
  GWEN_Init();
  DBG_ERROR(0, "Not yet supported");
  sleep(5);
  return 2;
}
