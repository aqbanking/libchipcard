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

#include "reader_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/types.h>
#include <chipcard2-client/chipcard2.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LCM_READER, LCM_Reader)
GWEN_LIST2_FUNCTIONS(LCM_READER, LCM_Reader)


LCM_READER *LCM_Reader_new(GWEN_TYPE_UINT32 serverId){
  LCM_READER *mr;

  GWEN_NEW_OBJECT(LCM_READER, mr);
  mr->serverId=serverId;
  mr->logBuffer=GWEN_Buffer_new(0, 512, 0, 1);

  return mr;
}


void LCM_Reader_free(LCM_READER *mr){
  if (mr) {
    free(mr->status);
    free(mr->readerId);
    free(mr->driverId);
    free(mr->readerName);
    free(mr->readerInfo);
    free(mr->shortDescr);
    GWEN_Buffer_free(mr->logBuffer);
    GWEN_FREE_OBJECT(mr);
  }
}



GWEN_TYPE_UINT32 LCM_Reader_GetServerId(const LCM_READER *mr){
  assert(mr);
  return mr->serverId;
}



const char *LCM_Reader_GetReaderId(const LCM_READER *mr){
  assert(mr);
  return mr->readerId;
}



void LCM_Reader_SetReaderId(LCM_READER *mr, const char *s){
  assert(mr);
  free(mr->readerId);
  if (s) mr->readerId=strdup(s);
  else mr->readerId=0;
  mr->lastChangeTime=time(0);
}



const char *LCM_Reader_GetDriverId(const LCM_READER *mr){
  assert(mr);
  return mr->driverId;
}



void LCM_Reader_SetDriverId(LCM_READER *mr, const char *s){
  assert(mr);
  free(mr->driverId);
  if (s) mr->driverId=strdup(s);
  else mr->driverId=0;
  mr->lastChangeTime=time(0);
}



const char *LCM_Reader_GetStatus(const LCM_READER *mr){
  assert(mr);
  return mr->status;
}



void LCM_Reader_SetStatus(LCM_READER *mr, const char *s){
  assert(mr);
  free(mr->status);
  if (s) mr->status=strdup(s);
  else mr->status=0;
  mr->lastChangeTime=time(0);
}



const char *LCM_Reader_GetReaderName(const LCM_READER *mr){
  assert(mr);
  return mr->readerName;
}



void LCM_Reader_SetReaderName(LCM_READER *mr, const char *s){
  assert(mr);
  free(mr->readerName);
  if (s) mr->readerName=strdup(s);
  else mr->readerName=0;
  mr->lastChangeTime=time(0);
}



const char *LCM_Reader_GetReaderInfo(const LCM_READER *mr){
  assert(mr);
  return mr->readerInfo;
}



void LCM_Reader_SetReaderInfo(LCM_READER *mr, const char *s){
  assert(mr);
  free(mr->readerInfo);
  if (s) mr->readerInfo=strdup(s);
  else mr->readerInfo=0;
  mr->lastChangeTime=time(0);
}



const char *LCM_Reader_GetReaderType(const LCM_READER *mr){
  assert(mr);
  return mr->readerType;
}



const char *LCM_Reader_GetShortDescr(const LCM_READER *mr){
  assert(mr);
  return mr->shortDescr;
}



void LCM_Reader_SetShortDescr(LCM_READER *mr, const char *s){
  assert(mr);
  free(mr->shortDescr);
  if (s) mr->shortDescr=strdup(s);
  else mr->shortDescr=0;
  mr->lastChangeTime=time(0);
}



void LCM_Reader_SetReaderType(LCM_READER *mr, const char *s){
  assert(mr);
  free(mr->readerType);
  if (s) mr->readerType=strdup(s);
  else mr->readerType=0;
  mr->lastChangeTime=time(0);
}



int LCM_Reader_GetReaderPort(const LCM_READER *mr){
  assert(mr);
  return mr->readerPort;
}



void LCM_Reader_SetReaderPort(LCM_READER *mr, int i){
  assert(mr);
  mr->readerPort=i;
  mr->lastChangeTime=time(0);
}



GWEN_TYPE_UINT32 LCM_Reader_GetReaderFlags(const LCM_READER *mr){
  assert(mr);
  return mr->readerFlags;
}



void LCM_Reader_SetReaderFlags(LCM_READER *mr, GWEN_TYPE_UINT32 i){
  assert(mr);
  mr->readerFlags=i;
  mr->lastChangeTime=time(0);
}



GWEN_BUFFER *LCM_Reader_GetLogBuffer(const LCM_READER *mr){
  assert(mr);
  return mr->logBuffer;
}



time_t LCM_Reader_GetLastChangeTime(const LCM_READER *mr){
  assert(mr);
  return mr->lastChangeTime;
}









