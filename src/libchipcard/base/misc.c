/***************************************************************************
    begin       : Tue Dec 23 2003
    copyright   : (C) 2021 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "libchipcard/chipcard.h"

#include <gwenhywfar/debug.h>

#include <assert.h>



uint32_t LC_ReaderFlags_fromDb(GWEN_DB_NODE *db, const char *name)
{
  int i;
  const char *p;
  uint32_t flags=0;

  for (i=0; ; i++) {
    p=GWEN_DB_GetCharValue(db, name, i, 0);
    if (!p)
      break;
    if (strcasecmp(p, "keypad")==0)
      flags|=LC_READER_FLAGS_KEYPAD;
    else if (strcasecmp(p, "display")==0)
      flags|=LC_READER_FLAGS_DISPLAY;
    else if (strcasecmp(p, "noinfo")==0)
      flags|=LC_READER_FLAGS_NOINFO;
    else if (strcasecmp(p, "remote")==0)
      flags|=LC_READER_FLAGS_REMOTE;
    else if (strcasecmp(p, "auto")==0)
      flags|=LC_READER_FLAGS_AUTO;
    else if (strcasecmp(p, "suspended_checks")==0)
      flags|=LC_READER_FLAGS_SUSPENDED_CHECKS;
    else if (strcasecmp(p, "driverHasVerify")==0)
      flags|=LC_READER_FLAGS_DRIVER_HAS_VERIFY;
    else if (strcasecmp(p, "keepRunning")==0)
      flags|=LC_READER_FLAGS_KEEP_RUNNING;
    else if (strcasecmp(p, "lowWriteBoundary")==0)
      flags|=LC_READER_FLAGS_LOW_WRITE_BOUNDARY;
    else if (strcasecmp(p, "noMemorySw")==0)
      flags|=LC_READER_FLAGS_NO_MEMORY_SW;
    else {
      DBG_WARN(0, "Unknown flag \"%s\", ignoring", p);
    }
  } /* for */

  return flags;
}



uint32_t LC_ReaderFlags_fromXml(GWEN_XMLNODE *node, const char *name)
{
  const char *p;
  uint32_t flags=0;
  GWEN_XMLNODE *n;

  n=GWEN_XMLNode_FindFirstTag(node, name, 0, 0);
  while (n) {
    GWEN_XMLNODE *nn;

    nn=GWEN_XMLNode_GetFirstData(n);
    if (nn) {
      p=GWEN_XMLNode_GetData(nn);
      assert(p);

      if (strcasecmp(p, "keypad")==0)
        flags|=LC_READER_FLAGS_KEYPAD;
      else if (strcasecmp(p, "display")==0)
        flags|=LC_READER_FLAGS_DISPLAY;
      else if (strcasecmp(p, "noinfo")==0)
        flags|=LC_READER_FLAGS_NOINFO;
      else if (strcasecmp(p, "remote")==0)
        flags|=LC_READER_FLAGS_REMOTE;
      else if (strcasecmp(p, "auto")==0)
        flags|=LC_READER_FLAGS_AUTO;
      else if (strcasecmp(p, "suspended_checks")==0)
        flags|=LC_READER_FLAGS_SUSPENDED_CHECKS;
      else if (strcasecmp(p, "driverHasVerify")==0)
        flags|=LC_READER_FLAGS_DRIVER_HAS_VERIFY;
      else if (strcasecmp(p, "keepRunning")==0)
        flags|=LC_READER_FLAGS_KEEP_RUNNING;
      else if (strcasecmp(p, "lowWriteBoundary")==0)
        flags|=LC_READER_FLAGS_LOW_WRITE_BOUNDARY;
      else if (strcasecmp(p, "noMemorySw")==0)
        flags|=LC_READER_FLAGS_NO_MEMORY_SW;
      else {
        DBG_WARN(0, "Unknown flag \"%s\", ignoring", p);
      }
    }
    n=GWEN_XMLNode_FindNextTag(n, name, 0, 0);
  } /* while */

  return flags;
}



void LC_ReaderFlags_toDb(GWEN_DB_NODE *db,
                         const char *name,
                         uint32_t fl)
{
  assert(db);
  assert(name);
  GWEN_DB_DeleteVar(db, name);
  if (fl & LC_READER_FLAGS_KEYPAD)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         name, "keypad");
  if (fl & LC_READER_FLAGS_DISPLAY)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         name, "display");
  if (fl & LC_READER_FLAGS_NOINFO)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         name, "noinfo");
  if (fl & LC_READER_FLAGS_REMOTE)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         name, "remote");
  if (fl & LC_READER_FLAGS_AUTO)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         name, "auto");
  if (fl & LC_READER_FLAGS_DRIVER_HAS_VERIFY)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         name, "driverHasVerify");
  if (fl & LC_READER_FLAGS_KEEP_RUNNING)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         name, "keepRunning");
  if (fl & LC_READER_FLAGS_LOW_WRITE_BOUNDARY)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         name, "lowWriteBoundary");
  if (fl & LC_READER_FLAGS_NO_MEMORY_SW)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         name, "noMemorySw");
}



