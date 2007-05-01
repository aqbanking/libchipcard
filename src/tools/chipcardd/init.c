/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: init.c 220 2006-09-08 13:00:00Z martin $
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

#include "chipcardd_p.h"
#include <gwenhywfar/directory.h>

#ifdef OS_WIN32
# define DIRSEP "\\"
#else
# define DIRSEP "/"
#endif



int init(ARGUMENTS *args) {
  GWEN_SSLCERTDESCR *cert;
  GWEN_BUFFER *fbuf;
  GWEN_TYPE_UINT32 pos;

  /* set some defaults */
  if (!args->commonName)
    args->commonName="chipcardd3";
  if (!args->certFile)
    args->certFile=LC_DEFAULT_CONFDIR "/chipcardd3.crt";
  if (!args->dhFile)
    args->dhFile=LC_DEFAULT_DHDIR;
  if (!args->countryName)
    args->countryName="DE";

  /* generate Diffie-Hellman stuff */
  fprintf(stderr, "Generating DH parameters, this may take some time.\n");
  fbuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Buffer_AppendString(fbuf, args->dhFile);

  if (GWEN_Directory_GetPath(args->dhFile, 0)) {
    DBG_ERROR(0, "Could not create/open path \"%s\"", args->dhFile);
    return 2;
  }

  GWEN_Buffer_AppendString(fbuf, DIRSEP);
  pos=GWEN_Buffer_GetPos(fbuf);

  fprintf(stderr, "- for 512 bits...\n");
  GWEN_Buffer_AppendString(fbuf, "dh-512.pem");
  if (GWEN_NetLayerSsl_GenerateDhFile(GWEN_Buffer_GetStart(fbuf), 512)) {
    fprintf(stderr, "ERROR: Could not generate DH file.\n");
    return 2;
  }
  GWEN_Buffer_Crop(fbuf, 0, pos);

  fprintf(stderr, "- for 1024 bits...\n");
  GWEN_Buffer_AppendString(fbuf, "dh-1024.pem");
  if (GWEN_NetLayerSsl_GenerateDhFile(GWEN_Buffer_GetStart(fbuf), 1024)) {
    fprintf(stderr, "ERROR: Could not generate DH file.\n");
    return 2;
  }
  GWEN_Buffer_Crop(fbuf, 0, pos);

  fprintf(stderr, "- for 2048 bits... (please wait, this is the last one)\n");
  GWEN_Buffer_AppendString(fbuf, "dh-2048.pem");
  if (GWEN_NetLayerSsl_GenerateDhFile(GWEN_Buffer_GetStart(fbuf), 2048)) {
    fprintf(stderr, "ERROR: Could not generate DH file.\n");
    return 2;
  }
  GWEN_Buffer_Crop(fbuf, 0, pos);

  /* generate certificate */
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


