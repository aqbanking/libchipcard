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


#ifndef LC_MON_READER_H
#define LC_MON_READER_H

/** @defgroup chipcardc_mon_reader Reader Information
 * @ingroup chipcardc_mon
 * @short Reader Information
 */
/*@{*/


typedef struct LCM_READER LCM_READER;

#include <chipcard/chipcard.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/list2.h>
#include <gwenhywfar/buffer.h>
#include <time.h>


GWEN_LIST_FUNCTION_LIB_DEFS(LCM_READER, LCM_Reader, CHIPCARD_API)
GWEN_LIST2_FUNCTION_LIB_DEFS(LCM_READER, LCM_Reader, CHIPCARD_API)


CHIPCARD_API
LCM_READER *LCM_Reader_new(uint32_t serverId);
CHIPCARD_API
void LCM_Reader_free(LCM_READER *mr);

CHIPCARD_API
uint32_t LCM_Reader_GetServerId(const LCM_READER *mr);
CHIPCARD_API
void LCM_Reader_SetServerId(LCM_READER *mr, uint32_t i);


CHIPCARD_API
const char *LCM_Reader_GetReaderId(const LCM_READER *mr);
CHIPCARD_API
void LCM_Reader_SetReaderId(LCM_READER *mr, const char *s);

CHIPCARD_API
const char *LCM_Reader_GetShortDescr(const LCM_READER *mr);
CHIPCARD_API
void LCM_Reader_SetShortDescr(LCM_READER *mr, const char *s);

CHIPCARD_API
const char *LCM_Reader_GetStatus(const LCM_READER *mr);
CHIPCARD_API
void LCM_Reader_SetStatus(LCM_READER *mr, const char *s);

CHIPCARD_API
const char *LCM_Reader_GetDriverId(const LCM_READER *mr);
CHIPCARD_API
void LCM_Reader_SetDriverId(LCM_READER *mr, const char *s);

CHIPCARD_API
const char *LCM_Reader_GetReaderType(const LCM_READER *mr);
CHIPCARD_API
void LCM_Reader_SetReaderType(LCM_READER *mr, const char *s);

CHIPCARD_API
const char *LCM_Reader_GetReaderName(const LCM_READER *mr);
CHIPCARD_API
void LCM_Reader_SetReaderName(LCM_READER *mr, const char *s);

CHIPCARD_API
const char *LCM_Reader_GetReaderInfo(const LCM_READER *mr);
CHIPCARD_API
void LCM_Reader_SetReaderInfo(LCM_READER *mr, const char *s);

CHIPCARD_API
int LCM_Reader_GetReaderPort(const LCM_READER *mr);
CHIPCARD_API
void LCM_Reader_SetReaderPort(LCM_READER *mr, int i);

CHIPCARD_API
uint32_t LCM_Reader_GetReaderFlags(const LCM_READER *mr);
CHIPCARD_API
void LCM_Reader_SetReaderFlags(LCM_READER *mr, uint32_t i);

CHIPCARD_API
GWEN_BUFFER *LCM_Reader_GetLogBuffer(const LCM_READER *mr);

CHIPCARD_API
time_t LCM_Reader_GetLastChangeTime(const LCM_READER *mr);

/*@}*/ /* defgroup */


#endif

