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

#include <stdio.h>


int main(int argc, char **argv) {

  fprintf(stderr,
          "%s",
          "===========================================================\n"
          "This tool is deprecated. Therefore it has been disabled.\n"
          "Please use gct-tool which comes with Gwenhywfar.\n"
          "===========================================================\n");
  return 3;
}


