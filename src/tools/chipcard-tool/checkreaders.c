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
#undef BUILDING_LIBCHIPCARD2_DLL


#include "global.h"
#include <time.h>
#include <assert.h>
#include <chipcard2-client/mon/monitor.h>
#include <gwenhywfar/debug.h>


int _checkReaders_check(LCM_MONITOR *mon) {
  LCM_SERVER *ms;

  assert(mon);
  ms=LCM_Server_List_First(LCM_Monitor_GetServers(mon));
  while(ms) {
    LCM_DRIVER *md;
    LCM_READER *mr;

    /* check drivers */
    md=LCM_Driver_List_First(LCM_Server_GetDrivers(ms));
    if (!md) {
      DBG_ERROR(0, "Still no drivers");
      return 1;
    }
    while(md) {
      if (strcasecmp(LCM_Driver_GetStatus(md),
                     LC_NOTIFY_CODE_DRIVER_UP)!=0 &&
          strcasecmp(LCM_Driver_GetStatus(md),
                     LC_NOTIFY_CODE_DRIVER_ERROR)!=0) {
        DBG_DEBUG(0, "Driver still down/started");
        return 1;
      }
      md=LCM_Driver_List_Next(md);
    }

    /* check readers */
    mr=LCM_Reader_List_First(LCM_Server_GetReaders(ms));
    if (!mr) {
      DBG_DEBUG(0, "Still no readers");
      return 1;
    }
    while(mr) {
      if (strcasecmp(LCM_Reader_GetStatus(mr),
                     LC_NOTIFY_CODE_READER_UP)!=0 &&
          strcasecmp(LCM_Reader_GetStatus(mr),
                     LC_NOTIFY_CODE_READER_ERROR)!=0) {
        DBG_INFO(0, "Reader still not up");
        return 1;
      }
      mr=LCM_Reader_List_Next(mr);
    }

    ms=LCM_Server_List_Next(ms);
  }
  return 0;
}



void _checkReaders_show(LCM_MONITOR *mon,
                        GWEN_DB_NODE *dbArgs) {
  LCM_SERVER *ms;
  int showReaders;
  int showDrivers;
  int showServices;
  int showDetailed;

  assert(mon);

  showDetailed=GWEN_DB_GetIntValue(dbArgs, "showAll", 0, 0);
  showDrivers=GWEN_DB_GetIntValue(dbArgs, "drivers", 0, 0);
  showServices=GWEN_DB_GetIntValue(dbArgs, "services", 0, 0);
  showReaders=GWEN_DB_GetIntValue(dbArgs, "readers", 0, 0);
  if (!showDrivers && !showServices)
    showReaders=1;

  ms=LCM_Server_List_First(LCM_Monitor_GetServers(mon));
  while(ms) {
    if (showServices) {
      LCM_SERVICE *mss;

      /* show services */
      mss=LCM_Service_List_First(LCM_Server_GetServices(ms));
      fprintf(stdout, "  Services:\n");
      while(mss) {
        fprintf(stdout,
                "  - %s (%0x8): %s\n",
                LCM_Service_GetServiceName(mss),
                LCM_Service_GetServiceId(mss),
                LCM_Service_GetStatus(mss));
        if (strcasecmp(LCM_Service_GetStatus(mss),
                       LC_NOTIFY_CODE_SERVICE_ERROR)==0 || showDetailed) {
          GWEN_BUFFER *lbuf;

          lbuf=LCM_Service_GetLogBuffer(mss);
          assert(lbuf);
          fprintf(stdout, "Event log:\n");
          fprintf(stdout, "%s", GWEN_Buffer_GetStart(lbuf));
        }
        mss=LCM_Service_List_Next(mss);
      } /* while mss */
    } /* if showServices */

    if (showDrivers) {
      LCM_DRIVER *md;

      /* show drivers */
      md=LCM_Driver_List_First(LCM_Server_GetDrivers(ms));
      fprintf(stdout, "  Drivers:\n");
      while(md) {
        fprintf(stdout,
                "  - %s (%s, %s): %s\n",
                LCM_Driver_GetDriverName(md),
                LCM_Driver_GetDriverType(md),
                LCM_Driver_GetLibraryFile(md),
                LCM_Driver_GetStatus(md));
        if (strcasecmp(LCM_Driver_GetStatus(md),
                       LC_NOTIFY_CODE_DRIVER_ERROR)==0 || showDetailed) {
          GWEN_BUFFER *lbuf;

          lbuf=LCM_Driver_GetLogBuffer(md);
          assert(lbuf);
          fprintf(stdout, "Event log:\n");
          fprintf(stdout, "%s", GWEN_Buffer_GetStart(lbuf));
        }
        md=LCM_Driver_List_Next(md);
      } /* while md */
    } /* if showReaders */

    if (showReaders) {
      LCM_READER *mr;

      /* show readers */
      mr=LCM_Reader_List_First(LCM_Server_GetReaders(ms));
      fprintf(stdout, "  Readers:\n");
      while(mr) {
        const char *ds;
        GWEN_TYPE_UINT32 rflags;

        ds=LCM_Reader_GetShortDescr(mr);
        if (!ds)
          ds=LCM_Reader_GetReaderType(mr);

        fprintf(stdout,
                "  - %s (%s, port %d",
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
        ds=LCM_Reader_GetReaderInfo(mr);
        if (ds) {
          fprintf(stdout, ", %s", ds);
        }
        fprintf(stdout, "): %s\n", LCM_Reader_GetStatus(mr));
        if (strcasecmp(LCM_Reader_GetStatus(mr),
                       LC_NOTIFY_CODE_READER_ERROR)==0 || showDetailed) {
          GWEN_BUFFER *lbuf;

          lbuf=LCM_Reader_GetLogBuffer(mr);
          assert(lbuf);
          fprintf(stdout, "Event log:\n");
          fprintf(stdout, "%s", GWEN_Buffer_GetStart(lbuf));
        }
        mr=LCM_Reader_List_Next(mr);
      } /* while mr */
    } /* if showReaders */

    ms=LCM_Server_List_Next(ms);
  }
}



int checkReaders(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs){
  LC_CLIENT_RESULT res;
  time_t startTime;
  LCM_MONITOR *mon;
  int timeOut;
  int doWait;
  GWEN_TYPE_UINT32 swId;

  doWait=GWEN_DB_VariableExists(dbArgs, "timeout");
  timeOut=GWEN_DB_GetIntValue(dbArgs, "timeout", 0, CHECKREADERS_TIMEOUT);

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

  swId=LC_Client_SendStartWait(cl, 0, 0);
  if (swId==0) {
    showError(0, LC_Client_ResultIpcError, "SendStartWait");
    return 2;
  }

  mon=LC_Client_GetMonitor(cl);
  assert(mon);
  startTime=time(0);

  while(1) {
    time_t currTime;

    currTime=time(0);
    if (difftime(currTime, startTime)>timeOut) {
      if (!doWait) {
	fprintf(stderr, "ERROR: Timeout.\n");
	return 2;
      }
      else {
	/* finished */
        _checkReaders_show(mon, dbArgs);
	return 0;
      }
    }
    res=LC_Client_Work_Wait(cl, 1);
    if (res==LC_Client_ResultOk || res==LC_Client_ResultWait) {
      int rv;

      rv=_checkReaders_check(mon);
      if (rv==0 && !doWait) {
        /* finished */
        _checkReaders_show(mon, dbArgs);
        return 0;
      }
      else if (rv==-1) {
        /* error */
      }
    }
    else {
      fprintf(stderr, "Error (%d)\n", res);
      break;
    }
  }

  return 0;
}




