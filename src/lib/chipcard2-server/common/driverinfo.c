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


#include "driverinfo.h"
#include <chipcard2/chipcard2.h>

#include <gwenhywfar/directory.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/debug.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>



int LC_DriverInfo_FindFile(GWEN_STRINGLIST *slDirs,
                           GWEN_STRINGLIST *slNames,
                           GWEN_BUFFER *nbuf) {
  GWEN_STRINGLISTENTRY *eDirs;

  eDirs=GWEN_StringList_FirstEntry(slDirs);
  while(eDirs) {
    GWEN_TYPE_UINT32 pos;
    GWEN_STRINGLISTENTRY *eNames;

    GWEN_Buffer_Reset(nbuf);
    GWEN_Buffer_AppendString(nbuf, GWEN_StringListEntry_Data(eDirs));
    GWEN_Buffer_AppendByte(nbuf, '/');
    pos=GWEN_Buffer_GetPos(nbuf);

    eNames=GWEN_StringList_FirstEntry(slNames);

    while(eNames) {
      GWEN_DIRECTORYDATA *dDir;

      dDir=GWEN_Directory_new();
      if (!GWEN_Directory_Open(dDir, GWEN_StringListEntry_Data(eDirs))) {
        char nameBuf[256];

        /* search for name in this folder */
        while(!GWEN_Directory_Read(dDir, nameBuf, sizeof(nameBuf))) {
          if (strcmp(nameBuf, ".")!=0 &&
              strcmp(nameBuf, "..")!=0) {
            if (-1!=GWEN_Text_ComparePattern(nameBuf,
                                             GWEN_StringListEntry_Data(eNames),
                                             0)) {
              struct stat st;

              /* found name, add it to the buffer */
              GWEN_Buffer_Crop(nbuf, 0, pos);
              GWEN_Buffer_SetPos(nbuf, pos);
              GWEN_Buffer_AppendString(nbuf, nameBuf);
              if (stat(GWEN_Buffer_GetStart(nbuf), &st)) {
                /* error */
                DBG_WARN(0, "stat(%s): %s",
                         GWEN_Buffer_GetStart(nbuf),
                         strerror(errno));
              }
              else {
                /* check for regular file */
                if (S_ISREG(st.st_mode)) {
                  GWEN_Directory_Close(dDir);
                  GWEN_Directory_free(dDir);
                  DBG_DEBUG(0, "File found: %s", GWEN_Buffer_GetStart(nbuf));
                  return 0;
                }
                else {
                  DBG_INFO(0, "Entry \"%s\" is not a regular file",
                           GWEN_Buffer_GetStart(nbuf));
                }
              }
            } /* if name pattern matches */
          } /* if not a special entry */
        } /* while still entries */
        GWEN_Directory_Close(dDir);
      }
      GWEN_Directory_free(dDir);

      eNames=GWEN_StringListEntry_Next(eNames);
    } /* while eNames */

    eDirs=GWEN_StringListEntry_Next(eDirs);
  } /* while eDirs */

  DBG_DEBUG(0, "File not found in search paths");
  return -1;
}



void LC_DriverInfo_SampleDirs(const char *dataDir, GWEN_STRINGLIST *sl) {
  GWEN_BUFFER *buf;
  GWEN_DIRECTORYDATA *d;
  unsigned int dpos;

  buf=GWEN_Buffer_new(0, 256, 0, 1);

  /* always append data dir */
  GWEN_StringList_AppendString(sl, dataDir, 0, 1);

  d=GWEN_Directory_new();
  GWEN_Buffer_AppendString(buf, dataDir);
  /*GWEN_Buffer_AppendByte(buf, '/');
  GWEN_Buffer_AppendString(buf, "drivers");*/

  dpos=GWEN_Buffer_GetPos(buf);
  if (!GWEN_Directory_Open(d, GWEN_Buffer_GetStart(buf))) {
    char buffer[256];

    while (!GWEN_Directory_Read(d, buffer, sizeof(buffer))){
      struct stat st;

      GWEN_Buffer_Crop(buf, 0, dpos);
      GWEN_Buffer_SetPos(buf, dpos);
      GWEN_Buffer_AppendByte(buf, '/');
      GWEN_Buffer_AppendString(buf, buffer);
      if (stat(GWEN_Buffer_GetStart(buf), &st)) {
        DBG_ERROR(0, "stat(%s): %s",
                  GWEN_Buffer_GetStart(buf),
                  strerror(errno));
      }
      else {
        if (S_ISDIR(st.st_mode)) {
          if (strcasecmp(buffer, "..")!=0 &&
              strcasecmp(buffer, ".")!=0) {
            DBG_DEBUG(0, "Adding driver dir \"%s\"",
                      GWEN_Buffer_GetStart(buf));
            GWEN_StringList_AppendString(sl,
                                         GWEN_Buffer_GetStart(buf),
                                         0, 1);
          } /* if real folder name */
        } /* if it is not a folder */
      } /* if stat succeeded */
    } /* while */
  } /* if open succeeded */
  else {
    DBG_ERROR(0, "Could not open folder %s", GWEN_Buffer_GetStart(buf));
  }
  GWEN_Directory_Close(d);
  GWEN_Directory_free(d);
  GWEN_Buffer_free(buf);
}



GWEN_DB_NODE *LC_DriverInfo_DriverDbFromXml(GWEN_XMLNODE *node) {
  GWEN_DB_NODE *db;
  GWEN_XMLNODE *n;
  GWEN_XMLNODE *nLib;
  const char *p;
  const char *dname;
  GWEN_STRINGLIST *slDirs;
  GWEN_STRINGLIST *slNames;
  GWEN_BUFFER *nbuf;

  db=GWEN_DB_Group_new("driver");
  dname=GWEN_XMLNode_GetProperty(node, "name", 0);
  if (!dname) {
    DBG_ERROR(0, "Driver in XML file has no name");
    GWEN_DB_Group_free(db);
    return 0;
  }
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
		       "driverName", dname);

  n=GWEN_XMLNode_FindFirstTag(node, "manufacturer", 0, 0);
  if (n) {
    p=GWEN_XMLNode_GetCharValue(n, "name", 0);
    if (p)
      GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                           "manufacturer", p);
    p=GWEN_XMLNode_GetCharValue(n, "url", 0);
    if (p)
      GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                           "url", p);
  }

  /* read variables */
  n=GWEN_XMLNode_FindFirstTag(node, "vars", "osname", OS_SHORTNAME);
  if (!n)
    n=GWEN_XMLNode_FindFirstTag(node, "vars", "ostype", OS_TYPE);
  if (!n)
    n=GWEN_XMLNode_FindFirstTag(node, "vars", "ostype", 0);
  if (!n) {
    n=GWEN_XMLNode_FindFirstTag(node, "vars", 0, 0);
    while(n) {
      if (GWEN_XMLNode_GetProperty(n, "osname", 0)==0 &&
          GWEN_XMLNode_GetProperty(n, "ostype", 0)==0)
        break;
      n=GWEN_XMLNode_FindNextTag(n, "vars", 0, 0);
    } /* while */
  }
  if (n) {
    GWEN_DB_NODE *dbVars;
    GWEN_XMLNODE *nn;

    dbVars=GWEN_DB_GetGroup(db, GWEN_DB_FLAGS_OVERWRITE_GROUPS, "vars");
    assert(dbVars);
    nn=GWEN_XMLNode_FindFirstTag(n, "var", 0, 0);
    while(nn) {
      const char *name;
      const char *value;
      GWEN_XMLNODE *nd;

      name=GWEN_XMLNode_GetProperty(nn, "name", 0);
      assert(name);

      nd=GWEN_XMLNode_GetFirstData(nn);
      if (nd)
	value=GWEN_XMLNode_GetData(nd);
      else
	value="";
      GWEN_DB_SetCharValue(dbVars, GWEN_DB_FLAGS_DEFAULT,
			   name, value);

      nn=GWEN_XMLNode_FindNextTag(nn, "var", 0, 0);
    } /* while */
  }


  p=GWEN_XMLNode_GetProperty(node, "type", 0);
  if (!p) {
    DBG_ERROR(0, "Driver \"%s\" in XML file has no type",
	      dname);
    GWEN_DB_Group_free(db);
    return 0;
  }
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
		       "driverType", p);

  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_DEFAULT,
		      "maxReaders",
		      atoi(GWEN_XMLNode_GetProperty(node,
						    "maxReaders",
						    "1")));

  p=GWEN_XMLNode_GetCharValue(node, "short", 0);
  if (p)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "short", p);

  nLib=GWEN_XMLNode_FindFirstTag(node, "lib", "osname", OS_SHORTNAME);
  if (!nLib)
    nLib=GWEN_XMLNode_FindFirstTag(node, "lib", "ostype", OS_TYPE);
  if (!nLib) {
    nLib=GWEN_XMLNode_FindFirstTag(node, "lib", 0, 0);
    while(nLib) {
      if (GWEN_XMLNode_GetProperty(nLib, "osname", 0)==0 &&
          GWEN_XMLNode_GetProperty(nLib, "ostype", 0)==0)
        break;
      nLib=GWEN_XMLNode_FindNextTag(nLib, "lib", 0, 0);
    } /* while */
  }
  if (!nLib) {
    DBG_ERROR(0, "No <lib> tag for driver \"%s\"", dname);
    GWEN_DB_Group_free(db);
    return 0;
  }

  /* fetch dirs */
  n=GWEN_XMLNode_FindFirstTag(nLib, "locations", 0, 0);
  if (!n) {
    DBG_ERROR(0, "No locations given for driver \"%s\"", dname);
    GWEN_DB_Group_free(db);
    return 0;
  }

  slDirs=GWEN_StringList_new();
  /* always add common lowlevel driver path to search list */
  nbuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Directory_OsifyPath(LC_LOWLEVELDRIVER_PATH, nbuf, 1);
  GWEN_StringList_AppendString(slDirs, GWEN_Buffer_GetStart(nbuf), 0, 1);
  GWEN_Buffer_free(nbuf);

  n=GWEN_XMLNode_FindFirstTag(n, "loc", 0, 0);
  while(n) {
    GWEN_XMLNODE *nData;

    nData=GWEN_XMLNode_GetFirstData(n);
    if (n) {
      p=GWEN_XMLNode_GetData(nData);
      if (p)
	GWEN_StringList_AppendString(slDirs,
				     p, 0, 1);
    }
    n=GWEN_XMLNode_FindNextTag(n, "loc", 0, 0);
  } /* while */

  /* fetch names */
  n=GWEN_XMLNode_FindFirstTag(nLib, "names", 0, 0);
  if (!n) {
    DBG_ERROR(0, "No names given for driver \"%s\"", dname);
    GWEN_StringList_free(slDirs);
    GWEN_DB_Group_free(db);
    return 0;
  }

  slNames=GWEN_StringList_new();
  n=GWEN_XMLNode_FindFirstTag(n, "name", 0, 0);
  while(n) {
    GWEN_XMLNODE *nData;

    nData=GWEN_XMLNode_GetFirstData(n);
    if (n) {
      p=GWEN_XMLNode_GetData(nData);
      if (p)
	GWEN_StringList_AppendString(slNames,
				     p, 0, 1);
    }
    n=GWEN_XMLNode_FindNextTag(n, "name", 0, 0);
  } /* while */

  nbuf=GWEN_Buffer_new(0, 256, 0, 1);
  if (!LC_DriverInfo_FindFile(slDirs, slNames, nbuf)) {
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "libraryFile", GWEN_Buffer_GetStart(nbuf));
  }
  GWEN_Buffer_free(nbuf);
  GWEN_StringList_free(slNames);
  GWEN_StringList_free(slDirs);

  /* read driver flags */
  n=GWEN_XMLNode_FindFirstTag(node, "flags", "osname", OS_SHORTNAME);
  if (!n)
    n=GWEN_XMLNode_FindFirstTag(node, "flags", "ostype", OS_TYPE);
  if (!n)
    n=GWEN_XMLNode_FindFirstTag(node, "flags", "ostype", 0);
  if (!n) {
    n=GWEN_XMLNode_FindFirstTag(node, "flags", 0, 0);
    while(n) {
      if (GWEN_XMLNode_GetProperty(n, "osname", 0)==0 &&
          GWEN_XMLNode_GetProperty(n, "ostype", 0)==0)
        break;
      n=GWEN_XMLNode_FindNextTag(n, "flags", 0, 0);
    } /* while */
  }
  if (n) {
    GWEN_XMLNODE *nn;

    nn=GWEN_XMLNode_FindFirstTag(n, "flag", 0, 0);
    while(nn) {
      const char *value;
      GWEN_XMLNODE *nd;

      nd=GWEN_XMLNode_GetFirstData(nn);
      if (nd)
	value=GWEN_XMLNode_GetData(nd);
      else
	value="";
      GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
			   "flags", value);

      nn=GWEN_XMLNode_FindNextTag(nn, "flag", 0, 0);
    } /* while */
  }

  return db;
}



GWEN_DB_NODE *LC_DriverInfo_ReaderDbFromXml(GWEN_XMLNODE *node) {
  GWEN_DB_NODE *db;
  GWEN_XMLNODE *n;
  const char *rtype;
  const char *p;
  int i;

  db=GWEN_DB_Group_new("reader");
  rtype=GWEN_XMLNode_GetProperty(node, "name", 0);
  if (!rtype) {
    DBG_ERROR(0, "Reader in XML file has no name");
    GWEN_DB_Group_free(db);
    return 0;
  }
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
		       "readerType", rtype);

  p=GWEN_XMLNode_GetCharValue(node, "short", rtype);
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                       "shortName", p);

  p=GWEN_XMLNode_GetProperty(node, "busType", "serial");
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                       "busType", p);

  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_DEFAULT,
		      "slots",
		      atoi(GWEN_XMLNode_GetProperty(node, "slots", "1")));

  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_DEFAULT,
                      "ctn",
                      atoi(GWEN_XMLNode_GetProperty(node, "ctn", "1")));

  if (1==sscanf(GWEN_XMLNode_GetProperty(node, "busId", "-1"),
                "%i", &i))
    if (i!=-1)
      GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_DEFAULT,
                          "busId", i);
  if (1==sscanf(GWEN_XMLNode_GetProperty(node, "vendor", "-1"),
                "%i", &i))
    if (i!=-1)
      GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_DEFAULT,
                          "vendorId", i);
  if (1==sscanf(GWEN_XMLNode_GetProperty(node, "product", "-1"),
                "%i", &i))
    if (i!=-1)
      GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_DEFAULT,
                          "productId", i);

  /* read flags */
  n=GWEN_XMLNode_FindNode(node, GWEN_XMLNodeTypeTag, "flags");
  if (n) {
    n=GWEN_XMLNode_FindFirstTag(n, "flag", 0, 0);
    while(n) {
      GWEN_XMLNODE *nData;

      nData=GWEN_XMLNode_GetFirstData(n);
      if (n) {
	p=GWEN_XMLNode_GetData(nData);
	if (p)
	  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
			       "flags", p);
      }
      n=GWEN_XMLNode_FindNextTag(n, "flag", 0, 0);
    } /* while */
  }

  /* read ports */
  n=GWEN_XMLNode_FindNode(node, GWEN_XMLNodeTypeTag, "ports");
  if (n) {
    GWEN_DB_NODE *dbPorts;

    dbPorts=GWEN_DB_GetGroup(db, GWEN_DB_FLAGS_OVERWRITE_GROUPS, "ports");
    n=GWEN_XMLNode_FindFirstTag(n, "port", 0, 0);
    while(n) {
      const char *vp;
      int i;

      vp=GWEN_XMLNode_GetProperty(n, "value", "0");
      if (1!=sscanf(vp, "%i", &i)) {
	DBG_ERROR(0, "Bad port value (%s), ignoring", vp);
      }
      else {
	GWEN_XMLNODE *nData;

	nData=GWEN_XMLNode_GetFirstData(n);
	if (nData) {
	  p=GWEN_XMLNode_GetData(nData);
	  if (p)
	    GWEN_DB_SetIntValue(dbPorts,
				GWEN_DB_FLAGS_DEFAULT,
				p, i);
	}
	else {
	  DBG_WARN(0, "No port name for value %d, ignoring", i);
	}
      }
      n=GWEN_XMLNode_FindNextTag(n, "port", 0, 0);
    } /* while */
  }

  /* read autoport setings */
  n=GWEN_XMLNode_FindNode(node, GWEN_XMLNodeTypeTag, "autoport");
  if (n) {
    GWEN_DB_NODE *dbAutoPort;
    const char *s;
    int i;

    dbAutoPort=GWEN_DB_GetGroup(db, GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                                "autoport");

    s=GWEN_XMLNode_GetCharValue(n, "mode", 0);
    if (s) {
      GWEN_DB_SetCharValue(dbAutoPort,
                           GWEN_DB_FLAGS_DEFAULT,
                           "mode", s);
    }

    /* split busorder (for mode=="pos") */
    s=GWEN_XMLNode_GetCharValue(n, "busorder", 0);
    if (s) {
      char *scopy;
      char *psc;

      scopy=strdup(s);

      psc=scopy;

      while(psc && *psc) {
        char *p;

        p=strchr(psc, ' ');
        if (p)
          *(p++)=0;
        GWEN_DB_SetCharValue(dbAutoPort,
                             GWEN_DB_FLAGS_DEFAULT,
                             "busorder", psc);
        psc=p;
      }
      free(scopy);
    }

    /* sortkey (for mode=="pos") */
    s=GWEN_XMLNode_GetCharValue(n, "sortkey", 0);
    if (s) {
      GWEN_DB_SetCharValue(dbAutoPort,
                           GWEN_DB_FLAGS_DEFAULT,
                           "sortkey", s);
    }

    /* offset */
    i=GWEN_XMLNode_GetIntValue(n, "offset", 0);
    GWEN_DB_SetIntValue(dbAutoPort,
                        GWEN_DB_FLAGS_DEFAULT,
                        "offset", i);

  }

  return db;
}





int LC_DriverInfo_SampleDrivers(GWEN_STRINGLIST *sl,
                                GWEN_DB_NODE *dbDrivers,
                                int availOnly) {
  GWEN_STRINGLISTENTRY *e;
  GWEN_BUFFER *nbuf;

  e=GWEN_StringList_FirstEntry(sl);
  nbuf=GWEN_Buffer_new(0, 256, 0, 1);
  while(e) {
    GWEN_XMLNODE *nFile;

    GWEN_Buffer_Reset(nbuf);
    GWEN_Buffer_AppendString(nbuf, GWEN_StringListEntry_Data(e));
    GWEN_Buffer_AppendString(nbuf, "/driver.xml");
    nFile=GWEN_XMLNode_new(GWEN_XMLNodeTypeTag, "drivers");
    if (GWEN_XML_ReadFile(nFile, GWEN_Buffer_GetStart(nbuf),
			  GWEN_XML_FLAGS_DEFAULT)) {
      DBG_INFO(0, "Could not read file \"%s\"", GWEN_Buffer_GetStart(nbuf));
    }
    else {
      GWEN_XMLNODE *nDriver;

      nDriver=GWEN_XMLNode_FindNode(nFile, GWEN_XMLNodeTypeTag, "driver");
      if (!nDriver) {
        DBG_INFO(0, "XML file \"%s\" does not contain a driver",
                 GWEN_Buffer_GetStart(nbuf));
      }
      else {
	GWEN_DB_NODE *dbDriver;

	dbDriver=LC_DriverInfo_DriverDbFromXml(nDriver);
	if (!dbDriver) {
	  DBG_INFO(0, "Could not create driver from file \"%s\"",
		   GWEN_Buffer_GetStart(nbuf));
	}
	else {
          if (GWEN_DB_GetCharValue(dbDriver, "libraryFile", 0, 0) ||
              !availOnly) {
            GWEN_XMLNODE *nReader;
  
	    nReader=GWEN_XMLNode_FindFirstTag(nDriver, "readers",
					      "osname", OS_SHORTNAME);
	    if (!nReader)
	      nReader=GWEN_XMLNode_FindFirstTag(nDriver, "readers",
						"ostype", OS_TYPE);
            if (!nReader) {
              nReader=GWEN_XMLNode_FindFirstTag(nDriver, "readers", 0, 0);
              while(nReader) {
                if (GWEN_XMLNode_GetProperty(nReader, "osname", 0)==0 &&
                    GWEN_XMLNode_GetProperty(nReader, "ostype", 0)==0)
                  break;
                nReader=GWEN_XMLNode_FindNextTag(nReader, "readers",
                                                 0, 0);
              } /* while */
            }
            if (!nReader) {
              DBG_INFO(0, "XML file \"%s\" contains no <readers> tag",
                       GWEN_Buffer_GetStart(nbuf));
            }
            else {
              int readers;
  
              readers=0;
              nReader=GWEN_XMLNode_FindFirstTag(nReader, "reader", 0, 0);
              while(nReader) {
                GWEN_DB_NODE *dbReader;
  
                dbReader=LC_DriverInfo_ReaderDbFromXml(nReader);
                if (dbReader) {
                  GWEN_DB_AddGroup(dbDriver, dbReader);
                  readers++;
                }
                nReader=GWEN_XMLNode_FindNextTag(nReader, "reader", 0, 0);
              } /* while */
              if (!readers) {
                DBG_INFO(0, "XML file \"%s\" contains no readers",
                         GWEN_Buffer_GetStart(nbuf));
              }
            }
            GWEN_DB_AddGroup(dbDrivers, dbDriver);
          }
          else {
            GWEN_DB_Group_free(dbDriver);
          }
	}
      }
    }
    GWEN_XMLNode_free(nFile);
    e=GWEN_StringListEntry_Next(e);
  } /* while eDirs */
  GWEN_Buffer_free(nbuf);

  return 0;
}




int LC_DriverInfo_ReadDrivers(const char *dataDir,
                              GWEN_DB_NODE *dbDrivers,
                              int availOnly) {
  GWEN_STRINGLIST *sl;
  int rv;

  sl=GWEN_StringList_new();
  LC_DriverInfo_SampleDirs(dataDir, sl);
  rv=LC_DriverInfo_SampleDrivers(sl, dbDrivers, availOnly);
  GWEN_StringList_free(sl);
  return rv;
}










