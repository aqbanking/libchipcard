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

#include "monitor_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/types.h>
#include <gwenhywfar/gwentime.h>
#include <chipcard2/chipcard2.h>
#include <chipcard2-client/client/card.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


LCM_MONITOR *LCM_Monitor_new(){
  LCM_MONITOR *mm;

  GWEN_NEW_OBJECT(LCM_MONITOR, mm);
  mm->servers=LCM_Server_List_new();
  return mm;
}



void LCM_Monitor_free(LCM_MONITOR *mm){
  if (mm) {
    LCM_Server_List_free(mm->servers);
    GWEN_FREE_OBJECT(mm);
  }
}



LCM_SERVER_LIST *LCM_Monitor_GetServers(const LCM_MONITOR *mm){
  assert(mm);
  return mm->servers;
}



void LCM_Monitor__LogToBuffer(GWEN_BUFFER *buf, const char *s) {
  GWEN_TIME *ti;
  char tbuffer[32];
  int hour, min, sec;

  assert(buf);
  if (!s)
    s="(null)";
  ti=GWEN_CurrentTime();
  assert(ti);
  if (GWEN_Time_GetBrokenDownTime(ti, &hour, &min, &sec))
    hour=min=sec=0;
  GWEN_Time_free(ti);
  snprintf(tbuffer, sizeof(tbuffer)-1, "%02d:%02d:%02d", hour, min, sec);
  tbuffer[sizeof(tbuffer)-1]=0;
  GWEN_Buffer_AppendString(buf, tbuffer);
  GWEN_Buffer_AppendByte(buf, ' ');
  GWEN_Buffer_AppendString(buf, s);
  GWEN_Buffer_AppendByte(buf, '\n');
}



int LCM_Monitor_HandleDriverNotification(LCM_MONITOR *mm,
                                         LCM_SERVER *ms,
                                         const LC_NOTIFICATION *n){
  const char *ncode;
  GWEN_DB_NODE *dbData;
  LCM_DRIVER *md;
  const char *driverId;
  const char *t;

  assert(mm);
  assert(ms);
  ncode=LC_Notification_GetCode(n);
  assert(ncode);
  dbData=LC_Notification_GetData(n);
  assert(dbData);
  driverId=GWEN_DB_GetCharValue(dbData, "driverId", 0, 0);
  assert(driverId);
  t=GWEN_DB_GetCharValue(dbData, "info", 0, 0);

  md=LCM_Driver_List_First(LCM_Server_GetDrivers(ms));
  while(md) {
    if (strcmp(driverId, LCM_Driver_GetDriverId(md))==0)
      break;
    md=LCM_Driver_List_Next(md);
  }
  if (!md) {
    const char *s;

    md=LCM_Driver_new(LCM_Server_GetServerId(ms));
    LCM_Driver_SetDriverId(md, driverId);
    s=GWEN_DB_GetCharValue(dbData, "driverType", 0, 0);
    assert(s);
    LCM_Driver_SetDriverType(md, s);
    s=GWEN_DB_GetCharValue(dbData, "driverName", 0, 0);
    assert(s);
    LCM_Driver_SetDriverName(md, s);
    s=GWEN_DB_GetCharValue(dbData, "libraryFile", 0, 0);
    assert(s);
    LCM_Driver_SetLibraryFile(md, s);
    LCM_Driver_List_Add(md, LCM_Server_GetDrivers(ms));
    mm->lastChangeTime=time(0);
    DBG_INFO(0, "Driver added");
  }

  if (strcasecmp(ncode, LC_NOTIFY_CODE_DRIVER_START)==0) {
    /* TODO: check all readers below */
    if (!t)
      t="driver started";
  }
  else if (strcasecmp(ncode, LC_NOTIFY_CODE_DRIVER_UP)==0) {
    /* TODO: check all readers below */
    if (!t)
      t="driver up";
  }
  else if (strcasecmp(ncode, LC_NOTIFY_CODE_DRIVER_DOWN)==0) {
    /* TODO: check all readers below */
    if (!t)
      t="driver down";
  }
  else if (strcasecmp(ncode, LC_NOTIFY_CODE_DRIVER_ERROR)==0) {
    /* TODO: check all readers below */
    if (!t)
      t="driver error";
  }
  else {
    DBG_ERROR(0, "Unhandled driver notification \"%s\"", ncode);
    return -1;
  }

  LCM_Driver_SetStatus(md, ncode);
  LCM_Monitor__LogToBuffer(LCM_Driver_GetLogBuffer(md), t);
  DBG_INFO(0, "Got a driver notification: %s - %s",
           LCM_Driver_GetDriverName(md), t);

  return 0;
}



int LCM_Monitor_HandleReaderNotification(LCM_MONITOR *mm,
                                         LCM_SERVER *ms,
                                         const LC_NOTIFICATION *n){
  const char *ncode;
  GWEN_DB_NODE *dbData;
  LCM_READER *mr;
  const char *readerId;
  const char *t;

  assert(mm);
  assert(ms);
  ncode=LC_Notification_GetCode(n);
  assert(ncode);
  dbData=LC_Notification_GetData(n);
  assert(dbData);
  readerId=GWEN_DB_GetCharValue(dbData, "readerId", 0, 0);
  assert(readerId);
  t=GWEN_DB_GetCharValue(dbData, "info", 0, 0);

  mr=LCM_Reader_List_First(LCM_Server_GetReaders(ms));
  while(mr) {
    if (strcmp(readerId, LCM_Reader_GetReaderId(mr))==0)
      break;
    mr=LCM_Reader_List_Next(mr);
  }
  if (!mr) {
    const char *s;
    unsigned int i;
    GWEN_TYPE_UINT32 f;

    mr=LCM_Reader_new(LCM_Server_GetServerId(ms));
    LCM_Reader_SetReaderId(mr, readerId);
    s=GWEN_DB_GetCharValue(dbData, "driverId", 0, 0);
    assert(s);
    LCM_Reader_SetDriverId(mr, s);
    s=GWEN_DB_GetCharValue(dbData, "readerType", 0, 0);
    assert(s);
    LCM_Reader_SetReaderType(mr, s);
    s=GWEN_DB_GetCharValue(dbData, "readerName", 0, 0);
    assert(s);
    LCM_Reader_SetReaderName(mr, s);
    s=GWEN_DB_GetCharValue(dbData, "readerInfo", 0, 0);
    if (s)
      LCM_Reader_SetReaderInfo(mr, s);
    LCM_Reader_SetReaderPort(mr,
                             GWEN_DB_GetIntValue(dbData, "readerPort", 0, 0));
    s=GWEN_DB_GetCharValue(dbData, "shortDescr", 0, 0);
    if (s)
      LCM_Reader_SetShortDescr(mr, s);

    /* get reader flags */
    f=0;
    for (i=0; ; i++) {
      const char *p;
  
      p=GWEN_DB_GetCharValue(dbData, "readerFlags", i, 0);
      if (!p)
        break;
      if (strcasecmp(p, "KEYPAD")==0)
        f|=LC_CARD_READERFLAGS_KEYPAD;
      else if (strcasecmp(p, "DISPLAY")==0)
        f|=LC_CARD_READERFLAGS_DISPLAY;
    } /* for */
    LCM_Reader_SetReaderFlags(mr, f);

    LCM_Reader_List_Add(mr, LCM_Server_GetReaders(ms));
    mm->lastChangeTime=time(0);
    DBG_INFO(0, "Reader added");
  }
  else {
    const char *s;

    s=GWEN_DB_GetCharValue(dbData, "readerInfo", 0, 0);
    if (s)
      LCM_Reader_SetReaderInfo(mr, s);
  }

  if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_START)==0) {
    /* TODO: check all cards below */
    if (!t)
      t="reader started";
  }
  else if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_UP)==0) {
    /* TODO: check all cards below */
    if (!t)
      t="reader up";
  }
  else if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_DOWN)==0) {
    /* TODO: check all cards below */
    if (!t)
      t="reader down";
  }
  else if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_ERROR)==0) {
    /* TODO: check all cards below */
    if (!t)
      t="reader error";
  }
  else {
    DBG_ERROR(0, "Unhandled reader notification \"%s\"", ncode);
    return -1;
  }

  LCM_Reader_SetStatus(mr, ncode);
  LCM_Monitor__LogToBuffer(LCM_Reader_GetLogBuffer(mr), t);
  DBG_INFO(0, "Got a reader notification: %s - %s",
           LCM_Reader_GetReaderName(mr), t);
  return 0;
}



int LCM_Monitor_HandleServiceNotification(LCM_MONITOR *mm,
                                          LCM_SERVER *ms,
                                          const LC_NOTIFICATION *n){
  const char *ncode;
  GWEN_DB_NODE *dbData;
  LCM_SERVICE *md;
  GWEN_TYPE_UINT32 serviceId;
  const char *t;

  assert(mm);
  assert(ms);
  ncode=LC_Notification_GetCode(n);
  assert(ncode);
  dbData=LC_Notification_GetData(n);
  assert(dbData);
  serviceId=0;
  if (1!=sscanf(GWEN_DB_GetCharValue(dbData, "serviceId", 0, "0"),
                "%x", &serviceId)) {
    DBG_ERROR(0, "Bad IPC message: Bad service id");
    abort();
  }
  assert(serviceId);
  t=GWEN_DB_GetCharValue(dbData, "info", 0, 0);

  md=LCM_Service_List_First(LCM_Server_GetServices(ms));
  while(md) {
    if (serviceId==LCM_Service_GetServiceId(md))
      break;
    md=LCM_Service_List_Next(md);
  }
  if (!md) {
    md=LCM_Service_new(LCM_Server_GetServerId(ms),
                       serviceId,
                       GWEN_DB_GetCharValue(dbData, "serviceName", 0, 0));
    LCM_Service_List_Add(md, LCM_Server_GetServices(ms));
    mm->lastChangeTime=time(0);
    DBG_INFO(0, "Service added");
  }

  if (strcasecmp(ncode, LC_NOTIFY_CODE_SERVICE_START)==0) {
    if (!t)
      t="service started";
  }
  else if (strcasecmp(ncode, LC_NOTIFY_CODE_SERVICE_UP)==0) {
    if (!t)
      t="service up";
  }
  else if (strcasecmp(ncode, LC_NOTIFY_CODE_SERVICE_DOWN)==0) {
    if (!t)
      t="service down";
  }
  else if (strcasecmp(ncode, LC_NOTIFY_CODE_SERVICE_ERROR)==0) {
    if (!t)
      t="service error";
  }
  else {
    DBG_ERROR(0, "Unhandled service notification \"%s\"", ncode);
    return -1;
  }

  LCM_Service_SetStatus(md, ncode);
  LCM_Monitor__LogToBuffer(LCM_Service_GetLogBuffer(md), t);
  DBG_INFO(0, "Got a service notification: %s - %s",
           LCM_Service_GetServiceName(md), t);

  return 0;
}




int LCM_Monitor_HandleNotification(LCM_MONITOR *mm,
                                   const LC_NOTIFICATION *n){
  const char *ntype;
  GWEN_TYPE_UINT32 serverId;
  LCM_SERVER *ms;
  int rv;

  assert(mm);

  /* get server or create it */
  serverId=LC_Notification_GetServerId(n);
  ms=LCM_Server_List_First(mm->servers);
  while(ms) {
    if (LCM_Server_GetServerId(ms)==serverId)
      break;
    ms=LCM_Server_List_Next(ms);
  }
  if (!ms) {
    ms=LCM_Server_new(serverId);
    LCM_Server_SetClientId(ms, LC_Notification_GetClientId(n));
    LCM_Server_List_Add(ms, mm->servers);
    mm->lastChangeTime=time(0);
    DBG_INFO(0, "Server added");
  }

  ntype=LC_Notification_GetType(n);
  assert(ntype);
  if (strcasecmp(ntype, LC_NOTIFY_TYPE_DRIVER)==0)
    rv=LCM_Monitor_HandleDriverNotification(mm, ms, n);
  else if (strcasecmp(ntype, LC_NOTIFY_TYPE_READER)==0)
    rv=LCM_Monitor_HandleReaderNotification(mm, ms, n);
  else if (strcasecmp(ntype, LC_NOTIFY_TYPE_SERVICE)==0)
    rv=LCM_Monitor_HandleServiceNotification(mm, ms, n);
  else {
    DBG_ERROR(0, "Unhandled notification type \"%s\"", ntype);
    rv=-1;
  }

  return rv;
}



LCM_SERVER *LCM_Monitor_FindServer(const LCM_MONITOR *mm,
                                   GWEN_TYPE_UINT32 serverId){
  LCM_SERVER *ms;

  assert(mm);
  ms=LCM_Server_List_First(mm->servers);
  while(ms) {
    if (LCM_Server_GetServerId(ms)==serverId)
      break;
    ms=LCM_Server_List_Next(ms);
  }

  return ms;
}



LCM_DRIVER *LCM_Monitor_FindDriver(const LCM_MONITOR *mm,
                                   GWEN_TYPE_UINT32 serverId,
                                   const char *driverId){
  LCM_SERVER *ms;
  LCM_DRIVER *md;

  ms=LCM_Monitor_FindServer(mm, serverId);
  if (!ms)
    return 0;

  md=LCM_Driver_List_First(LCM_Server_GetDrivers(ms));
  while(md) {
    if (strcmp(LCM_Driver_GetDriverId(md), driverId)==0)
      break;
    md=LCM_Driver_List_Next(md);
  }

  return md;
}



LCM_READER *LCM_Monitor_FindReader(const LCM_MONITOR *mm,
                                   GWEN_TYPE_UINT32 serverId,
                                   const char *readerId){
  LCM_SERVER *ms;
  LCM_READER *mr;

  ms=LCM_Monitor_FindServer(mm, serverId);
  if (!ms)
    return 0;

  mr=LCM_Reader_List_First(LCM_Server_GetReaders(ms));
  while(mr) {
    if (strcmp(LCM_Reader_GetReaderId(mr), readerId)==0)
      break;
    mr=LCM_Reader_List_Next(mr);
  }

  return mr;
}



time_t LCM_Monitor_GetLastChangeTime(const LCM_MONITOR *mm){
  assert(mm);
  return mm->lastChangeTime;
}












