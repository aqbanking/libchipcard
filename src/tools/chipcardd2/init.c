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

#include "chipcardd2_p.h"
#include <gwenhywfar/directory.h>


int init(ARGUMENTS *args) {
  GWEN_DB_NODE *db;

  /* set some defaults */
  if (!args->commonName)
    args->commonName="chipcardd2";
  if (!args->certFile)
    args->certFile=LC_DEFAULT_DATADIR "/chipcardd2.crt";
  if (!args->dhFile)
    args->dhFile=LC_DEFAULT_DATADIR "/" LC_DEFAULT_DHFILE;
  if (!args->countryName)
    args->countryName="DE";


  if (GWEN_Directory_GetPath(args->configFile,
                             GWEN_PATH_FLAGS_NAMEMUSTEXIST |
                             GWEN_PATH_FLAGS_VARIABLE)) {
    fprintf(stderr,
            I18N("ERROR: The configuration file \"%s\" does not exist.\n"
                 "Please copy the appropriate example file from\n"
                 "%s to %s\n"
                 "and adapt it to your needs.\n"
                ),
            args->configFile,
            LC_DEFAULT_DATADIR,
            args->configFile);
    return RETURNVALUE_SETUP;
  }

  /* generate Diffie-Hellman stuff */
  fprintf(stderr, "Generating DH parameters...\n");
  if (GWEN_NetTransportSSL_GenerateDhFile(args->dhFile, 1024)) {
    fprintf(stderr, "ERROR: Could not generate DH file.\n");
    return 2;
  }

  /* generate certificate */
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


