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


#include "global.h"
#include <time.h>
#include <assert.h>
#include <chipcard2-client/mon/monitor.h>
#include <gwenhywfar/debug.h>


void _listReaders_show(LCM_MONITOR *mon) {
  LCM_SERVER *ms;

  assert(mon);
  ms=LCM_Server_List_First(LCM_Monitor_GetServers(mon));
  while(ms) {
    LCM_DRIVER *md;
    LCM_READER *mr;

    /* show drivers */
    md=LCM_Driver_List_First(LCM_Server_GetDrivers(ms));
    fprintf(stdout, "Drivers:\n");
    while(md) {
      fprintf(stdout,
	      "- %s (%s, %s)\n",
              LCM_Driver_GetDriverName(md),
              LCM_Driver_GetDriverType(md),
              LCM_Driver_GetLibraryFile(md));
      md=LCM_Driver_List_Next(md);
    }

    /* show readers */
    mr=LCM_Reader_List_First(LCM_Server_GetReaders(ms));
    if (LCM_Driver_List_First(LCM_Server_GetDrivers(ms)))
      fprintf(stdout, "\n");
    fprintf(stdout, "Readers:\n");
    while(mr) {
      const char *ds;
      GWEN_TYPE_UINT32 rflags;

      ds=LCM_Reader_GetShortDescr(mr);
      if (!ds)
        ds=LCM_Reader_GetReaderType(mr);

      fprintf(stdout,
              "- %s (%s, port %d",
              LCM_Reader_GetReaderName(mr),
              ds,
              LCM_Reader_GetReaderPort(mr));
      rflags=LCM_Reader_GetReaderFlags(mr);
      if (rflags) {
        if (rflags & LC_CARD_READERFLAGS_KEYPAD)
          fprintf(stdout, ", keypad");
        if (rflags & LC_CARD_READERFLAGS_DISPLAY)
          fprintf(stdout, ", display");
      }
      fprintf(stdout, ")\n");
      mr=LCM_Reader_List_Next(mr);
    }

    ms=LCM_Server_List_Next(ms);
  }
}



int listReaders(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs){
  LC_CLIENT_RESULT res;
  LCM_MONITOR *mon;
  int verbosity;

  verbosity=GWEN_DB_GetIntValue(dbArgs, "verbosity", 0, 0);
  if (verbosity)
    fprintf(stderr, "Contacting server(s)...\n");
  res=LC_Client_SetNotify(cl,
                          LC_NOTIFY_FLAGS_READER_START|
                          LC_NOTIFY_FLAGS_READER_UP|
                          LC_NOTIFY_FLAGS_READER_DOWN|
                          LC_NOTIFY_FLAGS_READER_ERROR |
                          LC_NOTIFY_FLAGS_DRIVER_START|
                          LC_NOTIFY_FLAGS_DRIVER_UP|
                          LC_NOTIFY_FLAGS_DRIVER_DOWN|
                          LC_NOTIFY_FLAGS_DRIVER_ERROR);
  if (res!=LC_Client_ResultOk) {
    showError(0, res, "SetNotify");
    return 2;
  }

  mon=LC_Client_GetMonitor(cl);
  assert(mon);

  _listReaders_show(mon);
  return 0;
}




