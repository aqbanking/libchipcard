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

#include "driver_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/types.h>
#include <chipcard2/chipcard2.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LCM_DRIVER, LCM_Driver)
GWEN_LIST2_FUNCTIONS(LCM_DRIVER, LCM_Driver)




LCM_DRIVER *LCM_Driver_new(GWEN_TYPE_UINT32 serverId){
  LCM_DRIVER *md;

  GWEN_NEW_OBJECT(LCM_DRIVER, md);
  md->logBuffer=GWEN_Buffer_new(0, 512, 0, 1);
  md->serverId=serverId;
  return md;
}



void LCM_Driver_free(LCM_DRIVER *md){
  if (md) {
    GWEN_Buffer_free(md->logBuffer);
    free(md->driverId);
    free(md->driverType);
    free(md->driverName);
    free(md->libraryFile);
    GWEN_FREE_OBJECT(md);
  }
}



GWEN_TYPE_UINT32 LCM_Driver_GetServerId(const LCM_DRIVER *md){
  assert(md);
  return md->serverId;
}



const char *LCM_Driver_GetDriverId(const LCM_DRIVER *md){
  assert(md);
  return md->driverId;
}



void LCM_Driver_SetDriverId(LCM_DRIVER *md, const char *s){
  assert(md);
  free(md->driverId);
  if (s) md->driverId=strdup(s);
  else md->driverId=0;
  md->lastChangeTime=time(0);
}



const char *LCM_Driver_GetStatus(const LCM_DRIVER *md){
  assert(md);
  return md->status;
}



void LCM_Driver_SetStatus(LCM_DRIVER *md, const char *s){
  assert(md);
  free(md->status);
  if (s) md->status=strdup(s);
  else md->status=0;
  md->lastChangeTime=time(0);
}



const char *LCM_Driver_GetDriverType(const LCM_DRIVER *md){
  assert(md);
  return md->driverType;
}



void LCM_Driver_SetDriverType(LCM_DRIVER *md, const char *s){
  assert(md);
  free(md->driverType);
  if (s) md->driverType=strdup(s);
  else md->driverType=0;
  md->lastChangeTime=time(0);
}



const char *LCM_Driver_GetDriverName(const LCM_DRIVER *md){
  assert(md);
  return md->driverName;
}



void LCM_Driver_SetDriverName(LCM_DRIVER *md, const char *s){
  assert(md);
  free(md->driverName);
  if (s) md->driverName=strdup(s);
  else md->driverName=0;
  md->lastChangeTime=time(0);
}



const char *LCM_Driver_GetLibraryFile(const LCM_DRIVER *md){
  assert(md);
  return md->libraryFile;
}



void LCM_Driver_SetLibraryFile(LCM_DRIVER *md, const char *s){
  assert(md);
  free(md->libraryFile);
  if (s) md->libraryFile=strdup(s);
  else md->libraryFile=0;
  md->lastChangeTime=time(0);
}



GWEN_BUFFER *LCM_Driver_GetLogBuffer(const LCM_DRIVER *md){
  assert(md);
  return md->logBuffer;
}



time_t LCM_Driver_GetLastChangeTime(const LCM_DRIVER *md){
  assert(md);
  return md->lastChangeTime;
}













