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
#include <chipcard/sharedstuff/driverinfo.h>
#include <gwenhywfar/directory.h>


int addReader(ARGUMENTS *args) {
  GWEN_DB_NODE *dbConfig;
  GWEN_DB_NODE *dbKnownDrivers;
  GWEN_DB_NODE *dbDriver;
  GWEN_DB_NODE *dbReader;
  GWEN_DB_NODE *dbNewDriver;
  GWEN_DB_NODE *dbNewReader;
  GWEN_DB_NODE *dbDeviceManager;
  GWEN_DB_NODE *dbTmp;
  GWEN_BUFFER *nbuf;
  GWEN_BUFFER *dbuf;
  int rport;
  FILE *f;
  const char *s;
  int i;
  int rv;

  dbDriver=0;
  dbReader=0;
  if (args->dtype==0) {
    fprintf(stderr, "%s",
            I18N("ERROR: Driver type name not given.\n"
                 "Use \"--dtype list\" for a list of available drivers.\n")
           );
    return RETURNVALUE_PARAM;
  }

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
			 GWEN_PATH_FLAGS_CREATE_GROUP,
			 0, 2000)) {
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
				dbKnownDrivers, 0, 0)){
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

  /* GWEN_DB_Dump(dbKnownDrivers, stderr, 2); */

  if (strcasecmp(args->dtype, "list")==0) {
    GWEN_DB_NODE *dbT;

    dbT=GWEN_DB_FindFirstGroup(dbKnownDrivers, "driver");
    if (!dbT) {
      fprintf(stderr,
              I18N("ERROR: No drivers found.\n"
                   "(tried \"%s\")\n"
                   "It is most likely that chipcard3 was not\n"
                   "completely installed or the argument given to\n"
                   "\"--datadir\" does not represent a valid data folder.\n"
                  ),
              args->dataDir);
      GWEN_DB_Group_free(dbKnownDrivers);
      GWEN_DB_Group_free(dbConfig);
      return RETURNVALUE_SETUP;
    }

    while(dbT) {
      const char *dname;
      const char *dshort;

      dname=GWEN_DB_GetCharValue(dbT, "driverName", 0, 0);
      dshort=GWEN_DB_GetCharValue(dbT, "short", 0, 0);
      if (!dname || !dshort) {
        fprintf(stderr, "%s",
                I18N("WARN: Bad driver entry in XML driver files\n"
                    )
               );
      }
      else {
        if (GWEN_DB_GetCharValue(dbT, "libraryFile", 0, 0)==0) {
          fprintf(stdout,
                  I18N("%s (%s) [not installed]\n"),
                  dname, dshort);
        }
        else {
          fprintf(stdout,
                  I18N("%s (%s)\n"),
                  dname, dshort);
        }
      }
      dbT=GWEN_DB_FindNextGroup(dbT, "driver");
    }

    GWEN_DB_Group_free(dbKnownDrivers);
    GWEN_DB_Group_free(dbConfig);
    return 0;
  }

  /* search for driver */
  dbDriver=GWEN_DB_FindFirstGroup(dbKnownDrivers, "driver");
  if (!dbDriver) {
    fprintf(stderr,
            I18N("ERROR: No drivers found.\n"
                 "(tried \"%s\")\n"
                 "It is most likely that chipcard3 was not\n"
                 "completely installed or the argument given to\n"
                 "\"--datadir\" does not represent a valid data folder.\n"
                ),
            args->dataDir);
    GWEN_DB_Group_free(dbKnownDrivers);
    GWEN_DB_Group_free(dbConfig);
    return RETURNVALUE_SETUP;
  }

  while(dbDriver) {
    const char *dname;
    const char *dshort;

    dname=GWEN_DB_GetCharValue(dbDriver, "driverName", 0, 0);
    dshort=GWEN_DB_GetCharValue(dbDriver, "short", 0, 0);
    if (!dname || !dshort) {
      fprintf(stderr, "%s",
              I18N("WARN: Bad driver entry in XML driver files\n")
             );
    }
    else {
      if (strcasecmp(dname, args->dtype)==0)
        break;
    }
    dbDriver=GWEN_DB_FindNextGroup(dbDriver, "driver");
  }

  if (!dbDriver) {
    fprintf(stderr,
            I18N("ERROR: Driver \"%s\" not found.\n"
                 "Use \"--dtype list\" for a list of available drivers.\n"),
            args->dtype);
    GWEN_DB_Group_free(dbKnownDrivers);
    GWEN_DB_Group_free(dbConfig);
    return RETURNVALUE_PARAM;
  }

  if (args->rtype==0) {
    fprintf(stderr,
            I18N("ERROR: Reader type name not given.\n"
                 "Use \"--rtype list\" for a list of available readers\n"
                 "for driver \"%s\"\n"
                ),
            args->dtype);
    GWEN_DB_Group_free(dbKnownDrivers);
    GWEN_DB_Group_free(dbConfig);
    return RETURNVALUE_PARAM;
  }

  if (strcasecmp(args->rtype, "list")==0) {
    GWEN_DB_NODE *dbT;

    dbT=GWEN_DB_FindFirstGroup(dbDriver, "reader");
    if (!dbT) {
      fprintf(stderr,
              I18N("ERROR: No readers found for driver \"%s\".\n"
                  ),
              args->dtype);
      GWEN_DB_Group_free(dbKnownDrivers);
      GWEN_DB_Group_free(dbConfig);
      return RETURNVALUE_SETUP;
    }

    while(dbT) {
      const char *rname;
      const char *rshort;

      rname=GWEN_DB_GetCharValue(dbT, "readerType", 0, 0);
      rshort=GWEN_DB_GetCharValue(dbT, "shortName", 0, 0);
      if (!rname || !rshort) {
        fprintf(stderr, "%s",
                I18N("WARN: Bad reader entry in XML driver files\n"
                    )
               );
      }
      else {
        fprintf(stdout,
                I18N("%s (%s)\n"),
                rname, rshort);
      }
      dbT=GWEN_DB_FindNextGroup(dbT, "reader");
    }

    GWEN_DB_Group_free(dbKnownDrivers);
    GWEN_DB_Group_free(dbConfig);
    return 0;
  }

  /* search for reader */
  dbReader=GWEN_DB_FindFirstGroup(dbDriver, "reader");
  if (!dbReader) {
    fprintf(stderr,
            I18N("ERROR: No readers found for driver \"%s\".\n"
                ),
            args->dtype);
    GWEN_DB_Group_free(dbKnownDrivers);
    GWEN_DB_Group_free(dbConfig);
   return RETURNVALUE_SETUP;
  }

  while(dbReader) {
    const char *rname;
    const char *rshort;

    rname=GWEN_DB_GetCharValue(dbReader, "readerType", 0, 0);
    rshort=GWEN_DB_GetCharValue(dbReader, "shortName", 0, 0);
    if (!rname || !rshort) {
      fprintf(stderr, "%s",
              I18N("WARN: Bad driver entry in XML driver files\n")
             );
    }
    else {
      if (strcasecmp(rname, args->rtype)==0)
        break;
    }
    dbReader=GWEN_DB_FindNextGroup(dbReader, "reader");
  }

  if (!dbReader) {
    fprintf(stderr,
            I18N("ERROR: Reader \"%s\" not found in driver \"%s\".\n"
                 "Use \"--rtype list\" for a list of available readers\n"
                 "for the driver.\n"),
            args->rtype, args->dtype);
    GWEN_DB_Group_free(dbKnownDrivers);
    GWEN_DB_Group_free(dbConfig);
    return RETURNVALUE_PARAM;
  }

  if (args->rname==0) {
    fprintf(stderr,
            I18N("ERROR: No reader name given (use \"--rname ARG\")\n")
           );
    GWEN_DB_Group_free(dbKnownDrivers);
    GWEN_DB_Group_free(dbConfig);
    return RETURNVALUE_PARAM;
  }

  if (GWEN_DB_GetCharValue(dbDriver, "libraryFile", 0, 0)==0) {
    const char *url;

    fprintf(stderr, "%s",
            I18N("ERROR: The low-level driver provided by the manufacturer\n"
                 "is not installed on your system.\n")
           );
    url=GWEN_DB_GetCharValue(dbDriver, "url", 0, 0);
    if (url)
      fprintf(stderr,
              I18N("Please visit the manufacturer's site at\n"
                   "     %s\n"
                   "to download the driver.\n"),
              url);

    GWEN_DB_Group_free(dbKnownDrivers);
    GWEN_DB_Group_free(dbConfig);
    return RETURNVALUE_SETUP;
  }

  if (strcasecmp(GWEN_DB_GetCharValue(dbReader, "bustype", 0, "serial"),
                 "serial")==0) {
    GWEN_DB_NODE *dbT;

    /* --rport is needed for serial readers */
    if (args->rport==0) {
      fprintf(stderr,
              I18N("ERROR: No port given for serial reader\n"
                   "Use \"--rport list\" for a list of available ports.\n")
             );
      GWEN_DB_Group_free(dbKnownDrivers);
      GWEN_DB_Group_free(dbConfig);
      return RETURNVALUE_PARAM;
    }

    dbT=GWEN_DB_GetGroup(dbReader, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                         "ports");
    if (!dbT) {
      fprintf(stderr, "%s",
              I18N("ERROR: No ports specified in driver XML file\n")
             );
      GWEN_DB_Group_free(dbKnownDrivers);
      GWEN_DB_Group_free(dbConfig);
      return RETURNVALUE_SETUP;
    }

    if (strcasecmp(args->rport, "list")==0) {
      GWEN_DB_NODE *dbPort;

      dbPort=GWEN_DB_GetFirstVar(dbT);
      if (dbPort) {
        while(dbPort) {
          const char *s;

          s=GWEN_DB_VariableName(dbPort);
          assert(s);
          fprintf(stdout, " %s", s);
          dbPort=GWEN_DB_GetNextVar(dbPort);
        }
        fprintf(stdout, "\n");
      }
      else {
        fprintf(stderr, "%s",
                I18N("ERROR: No ports specified in driver XML file\n"));
        GWEN_DB_Group_free(dbKnownDrivers);
        GWEN_DB_Group_free(dbConfig);
        return RETURNVALUE_SETUP;
      }
      GWEN_DB_Group_free(dbKnownDrivers);
      GWEN_DB_Group_free(dbConfig);
      return 0;
    }

    rport=GWEN_DB_GetIntValue(dbT, args->rport, 0, -1);
    if (rport==-1) {
      fprintf(stderr,
              I18N("ERROR: Port \"%s\" not found in reader's XML "
                   "description\n"),
              args->rport);
      GWEN_DB_Group_free(dbKnownDrivers);
      GWEN_DB_Group_free(dbConfig);
      return RETURNVALUE_PARAM;
    }
  }
  else {
    /* USB readers are auto-detected */
    fprintf(stderr, "%s",
            I18N("ERROR: You must not add USB readers, they are added\n"
                 "automatically upon startup of the chipcard daemon.\n")
           );
    GWEN_DB_Group_free(dbKnownDrivers);
    GWEN_DB_Group_free(dbConfig);
    return RETURNVALUE_PARAM;
  }

  /* check whether reader already exists in config file */
  dbTmp=GWEN_DB_FindFirstGroup(dbConfig, "driver");
  while(dbTmp) {
    GWEN_DB_NODE *dbT;

    dbT=GWEN_DB_FindFirstGroup(dbTmp, "reader");
    while(dbT) {
      s=GWEN_DB_GetCharValue(dbT, "readerName", 0, 0);
      if (s) {
        if (strcasecmp(s, args->rname)==0) {
          fprintf(stderr,
                  I18N("ERROR: A reader with the name \"%s\" already "
                       "exists.\n"),
                  args->rname);
          GWEN_DB_Group_free(dbKnownDrivers);
          GWEN_DB_Group_free(dbConfig);
          return RETURNVALUE_PARAM;
        }
      }
      dbT=GWEN_DB_FindNextGroup(dbT, "reader");
    }
    dbTmp=GWEN_DB_FindNextGroup(dbTmp, "driver");
  }


  dbNewDriver=GWEN_DB_Group_new("driver");
  s=GWEN_DB_GetCharValue(dbDriver, "driverType", 0, 0);
  assert(s);
  GWEN_DB_SetCharValue(dbNewDriver, GWEN_DB_FLAGS_DEFAULT, "driverType", s);
  s=GWEN_DB_GetCharValue(dbDriver, "driverName", 0, 0);
  assert(s);
  GWEN_DB_SetCharValue(dbNewDriver, GWEN_DB_FLAGS_DEFAULT, "driverName", s);
  s=GWEN_DB_GetCharValue(dbDriver, "libraryFile", 0, 0);
  assert(s);
  GWEN_DB_SetCharValue(dbNewDriver, GWEN_DB_FLAGS_DEFAULT, "libraryFile", s);

  dbNewReader=GWEN_DB_GetGroup(dbNewDriver, GWEN_PATH_FLAGS_CREATE_GROUP,
                               "reader");
  s=GWEN_DB_GetCharValue(dbReader, "readerType", 0, 0);
  assert(s);
  GWEN_DB_SetCharValue(dbNewReader, GWEN_DB_FLAGS_DEFAULT, "readerType", s);
  GWEN_DB_SetCharValue(dbNewReader, GWEN_DB_FLAGS_DEFAULT, "readerName",
                       args->rname);
  GWEN_DB_SetIntValue(dbNewReader, GWEN_DB_FLAGS_DEFAULT, "port", rport);
  GWEN_DB_SetIntValue(dbNewReader, GWEN_DB_FLAGS_DEFAULT, "slots",
                      GWEN_DB_GetIntValue(dbReader, "slots", 0, 1));
  GWEN_DB_SetCharValue(dbNewReader, GWEN_DB_FLAGS_DEFAULT, "busType",
                       GWEN_DB_GetCharValue(dbReader, "busType",0,"serial"));
  for (i=0; ; i++) {
    s=GWEN_DB_GetCharValue(dbReader, "flags", i, 0);
    if (!s)
      break;
    GWEN_DB_SetCharValue(dbNewReader, GWEN_DB_FLAGS_DEFAULT, "flags", s);
  }

  dbDeviceManager=GWEN_DB_GetGroup(dbConfig, GWEN_DB_FLAGS_DEFAULT,
                                   "DeviceManager");
  assert(dbDeviceManager);
  GWEN_DB_AddGroup(dbDeviceManager, dbNewDriver);

  if (GWEN_Directory_GetPath(args->configFile, GWEN_PATH_FLAGS_VARIABLE)) {
    fprintf(stderr,
            I18N("ERROR: Could not create configuration file \"%s\".\n"),
            args->configFile);
    GWEN_DB_Group_free(dbKnownDrivers);
    GWEN_DB_Group_free(dbConfig);
    return RETURNVALUE_SETUP;
  }

  if (GWEN_DB_WriteFile(dbConfig,
			GWEN_Buffer_GetStart(nbuf),
			GWEN_DB_FLAGS_DEFAULT,
			0, 5000)) {
    fprintf(stderr,
            I18N("ERROR: Could not save configuration file \"%s\".\n"),
            args->configFile);
    GWEN_DB_Group_free(dbKnownDrivers);
    GWEN_DB_Group_free(dbConfig);
    return RETURNVALUE_SETUP;
  }

  GWEN_DB_Group_free(dbKnownDrivers);
  GWEN_DB_Group_free(dbConfig);
  return 0;
}











