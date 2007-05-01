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


#ifndef LC_MON_SERVICE_H
#define LC_MON_SERVICE_H

/** @defgroup chipcardc_mon_service Service Information
 * @ingroup chipcardc_mon
 * @short Service Information
 */
/*@{*/


typedef struct LCM_SERVICE LCM_SERVICE;

#include <chipcard3/chipcard3.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/list2.h>
#include <gwenhywfar/buffer.h>
#include <time.h>




GWEN_LIST_FUNCTION_LIB_DEFS(LCM_SERVICE, LCM_Service, CHIPCARD_API)
GWEN_LIST2_FUNCTION_LIB_DEFS(LCM_SERVICE, LCM_Service, CHIPCARD_API)


CHIPCARD_API
LCM_SERVICE *LCM_Service_new(GWEN_TYPE_UINT32 serverId,
                             GWEN_TYPE_UINT32 serviceId,
                             const char *serviceName);
CHIPCARD_API
void LCM_Service_free(LCM_SERVICE *ms);

/**
 */
CHIPCARD_API
GWEN_TYPE_UINT32 LCM_Service_GetServiceId(const LCM_SERVICE *ms);

CHIPCARD_API
GWEN_TYPE_UINT32 LCM_Service_GetServerId(const LCM_SERVICE *ms);

CHIPCARD_API
const char *LCM_Service_GetServiceName(const LCM_SERVICE *ms);


CHIPCARD_API
GWEN_BUFFER *LCM_Service_GetLogBuffer(const LCM_SERVICE *ms);

CHIPCARD_API
time_t LCM_Service_GetLastChangeTime(const LCM_SERVICE *ms);

CHIPCARD_API
const char *LCM_Service_GetStatus(const LCM_SERVICE *ms);
CHIPCARD_API
void LCM_Service_SetStatus(LCM_SERVICE *ms, const char *s);


/*@}*/ /* defgroup */

#endif

