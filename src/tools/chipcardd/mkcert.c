/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: mkcert.c 220 2006-09-08 13:00:00Z martin $
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


int mkCert(ARGUMENTS *args) {
  GWEN_SSLCERTDESCR *cert;

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
  cert=GWEN_SslCertDescr_new();
  if (args->countryName)
    GWEN_SslCertDescr_SetCountryName(cert, args->countryName);
  if (args->commonName)
    GWEN_SslCertDescr_SetCommonName(cert, args->commonName);

  if (GWEN_NetLayerSsl_GenerateCertAndKeyFile(args->certFile,
                                              1024,
                                              1,
                                              365*2,
                                              cert)) {
    fprintf(stderr, "ERROR: Could not generate certificate.\n");
    return 2;
  }

  fprintf(stderr, "Done.\n");
  return 0;
}



