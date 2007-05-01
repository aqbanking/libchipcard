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


#ifndef LC_MON_DRIVER_H
#define LC_MON_DRIVER_H

/** @defgroup chipcardc_mon_driver Driver Information
 * @ingroup chipcardc_mon
 * @short Driver Information
 */
/*@{*/

typedef struct LCM_DRIVER LCM_DRIVER;

#include <gwenhywfar/misc.h>
#include <gwenhywfar/list2.h>
#include <gwenhywfar/buffer.h>
#include <time.h>



GWEN_LIST_FUNCTION_LIB_DEFS(LCM_DRIVER, LCM_Driver, CHIPCARD_API)
GWEN_LIST2_FUNCTION_LIB_DEFS(LCM_DRIVER, LCM_Driver, CHIPCARD_API)


CHIPCARD_API
LCM_DRIVER *LCM_Driver_new(GWEN_TYPE_UINT32 serverId);
CHIPCARD_API
void LCM_Driver_free(LCM_DRIVER *md);

CHIPCARD_API
GWEN_TYPE_UINT32 LCM_Driver_GetServerId(const LCM_DRIVER *md);
CHIPCARD_API
const char *LCM_Driver_GetDriverId(const LCM_DRIVER *md);
CHIPCARD_API
void LCM_Driver_SetDriverId(LCM_DRIVER *md, const char *s);

CHIPCARD_API
const char *LCM_Driver_GetStatus(const LCM_DRIVER *md);
CHIPCARD_API
void LCM_Driver_SetStatus(LCM_DRIVER *md, const char *s);

CHIPCARD_API
const char *LCM_Driver_GetDriverType(const LCM_DRIVER *md);
CHIPCARD_API
void LCM_Driver_SetDriverType(LCM_DRIVER *md, const char *s);

CHIPCARD_API
const char *LCM_Driver_GetDriverName(const LCM_DRIVER *md);
CHIPCARD_API
void LCM_Driver_SetDriverName(LCM_DRIVER *md, const char *s);

CHIPCARD_API
const char *LCM_Driver_GetLibraryFile(const LCM_DRIVER *md);
CHIPCARD_API
void LCM_Driver_SetLibraryFile(LCM_DRIVER *md, const char *s);

CHIPCARD_API
GWEN_BUFFER *LCM_Driver_GetLogBuffer(const LCM_DRIVER *md);

CHIPCARD_API
time_t LCM_Driver_GetLastChangeTime(const LCM_DRIVER *md);

/*@}*/ /* defgroup */


#endif

