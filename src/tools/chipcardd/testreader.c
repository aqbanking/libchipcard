/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: testreader.c 220 2006-09-08 13:00:00Z martin $
    begin       : Sun May 30 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#undef BUILDING_LIBCHIPCARD2_DLL


/* Internationalization */
#ifdef HAVE_GETTEXT_ENVIRONMENT
# include <libintl.h>
# include <locale.h>
# define I18N(m) gettext(m)
#else
# define I18N(m) m
#endif
#define I18NT(m) m

#include "chipcardd_p.h"


int testReader(ARGUMENTS *args) {
  fprintf(stderr, "ERROR: Not yet implemented.\n");
  return RETURNVALUE_PARAM;
}


