/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id$
    begin       : Sun May 30 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


/* Internationalization */
#ifdef HAVE_GETTEXT_ENVIRONMENT
# include <libintl.h>
# include <locale.h>
# define I18N(m) gettext(m)
#else
# define I18N(m) m
#endif
#define I18NT(m) m

#include "chipcardd2_p.h"


int mkCert(ARGUMENTS *args) {
  GWEN_DB_NODE *db;

  if (!args->commonName) {
    fprintf(stderr,
            "ERROR: option \"--user USER\" needed for certificate.\n");
    return 1;
  }
  if (!args->certFile) {
    fprintf(stderr,
            "ERROR: option \"--certfile FILE\" needed for certificate.\n");
    return 1;
  }
  if (!args->countryName)
    args->countryName="DE";

  fprintf(stderr, "Generating self-signed certificate for server...\n");
  db=GWEN_DB_Group_new("certData");
  if (args->countryName)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "countryName", args->countryName);
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                       "commonName", args->commonName);
  if (GWEN_NetTransportSSL_GenerateCertAndKeyFile(args->certFile,
                                                  1024,
                                                  1,
                                                  365*2,
                                                  db)) {
    fprintf(stderr, "ERROR: Could not generate certificate.\n");
    return 2;
  }

  fprintf(stderr, "Done.\n");
  return 0;
}



