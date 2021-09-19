/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2021 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD2_MSGENGINE_H
#define CHIPCARD2_MSGENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libchipcard/chipcard.h>

#include <gwenhywfar/msgengine.h>

/**
 * @file sharedstuff/msgengine.h
 *
 * This message engine implements a few new types:
 * <ul>
 *   <li>byte</li>
 *   <li>word (bigEndian="1")</li>
 *   <li>dword (bigEndian="1")</li>
 *   <li>bytes (size="-1") </li>
 *   <li>tlv (tlvType="BER"||"SIMPLE") </li>
 * </ul>
 */

CHIPCARD_API
GWEN_MSGENGINE *LC_MsgEngine_new();


#ifdef __cplusplus
}
#endif


#endif /* CHIPCARD2_MSGENGINE_H */


