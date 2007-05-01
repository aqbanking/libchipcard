/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: addreader.c 219 2006-09-08 12:57:36Z martin $
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
#include "common/driverinfo.h"


int listReaders(ARGUMENTS *args) {
  GWEN_DB_NODE *dbConfig;
  GWEN_DB_NODE *dbKnownDrivers;
  GWEN_BUFFER *nbuf;
  GWEN_BUFFER *dbuf;
  FILE *f;
  int rv;
  GWEN_DB_NODE *dbD;

  nbuf=GWEN_Buffer_new(0, 256, 0, 1);
  rv=getConfigFile(args, nbuf);
  if (rv!=0) {
    fprintf(stderr,
            "No configuration file found.\n"
            "Please either specify one or copy the example file.\n");
    return RETURNVALUE_PARAM;
  }

  dbConfig=GWEN_DB_Group_new("config");
  f=fopen(GWEN_Buffer_GetStart(nbuf), "r");
  if (f) {
    fclose(f);
    if (GWEN_DB_ReadFile(dbConfig,
                         GWEN_Buffer_GetStart(nbuf),
                         GWEN_DB_FLAGS_DEFAULT |
                         GWEN_PATH_FLAGS_CREATE_GROUP)) {
      fprintf(stderr,
              I18N("ERROR: Bad configuration file \"%s\"\n"),
              GWEN_Buffer_GetStart(nbuf));
      GWEN_DB_Group_free(dbConfig);
      return RETURNVALUE_SETUP;
    }
  }
  else {
    fprintf(stderr,
            I18N("WARNING: Configuration file \"%s\" does not exist.\n"
                 "Please try the command \"init\" first.\n"),
            GWEN_Buffer_GetStart(nbuf));
  }

  /* read drivers */
  dbKnownDrivers=GWEN_DB_Group_new("drivers");
  dbuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Buffer_AppendString(dbuf, args->dataDir);
  GWEN_Buffer_AppendString(dbuf, "/drivers");
  if (LC_DriverInfo_ReadDrivers(GWEN_Buffer_GetStart(dbuf),
                                dbKnownDrivers, 0)){
    fprintf(stderr,
            I18N("Could not read the driver list\n"
                 "(tried \"%s\")\n"
                ),
            args->dataDir);
    GWEN_DB_Group_free(dbKnownDrivers);
    GWEN_DB_Group_free(dbConfig);
    return RETURNVALUE_SETUP;
  }
  GWEN_Buffer_free(dbuf);


  dbD=GWEN_DB_FindFirstGroup(dbKnownDrivers, "driver");
  while(dbD) {
    GWEN_DB_NODE *dbR;

    dbR=GWEN_DB_FindFirstGroup(dbD, "reader");
    while(dbR) {
      const char *rname;
      const char *rbus;
      int vendorId;
      int productId;

      rname=GWEN_DB_GetCharValue(dbR, "readerType", 0, 0);
      rbus=GWEN_DB_GetCharValue(dbR, "busType", 0, 0);
      vendorId=GWEN_DB_GetIntValue(dbR, "vendorId", 0, 0);
      productId=GWEN_DB_GetIntValue(dbR, "productId", 0, 0);
      if (rname && rbus && vendorId && productId) {
        printf("%s\t%s\t%04x\t%04x\n",
               rname, rbus, vendorId, productId);
      }
      dbR=GWEN_DB_FindNextGroup(dbR, "reader");
    }

    dbD=GWEN_DB_FindNextGroup(dbD, "driver");
  }

  GWEN_DB_Group_free(dbKnownDrivers);
  GWEN_DB_Group_free(dbConfig);
  return 0;
}


