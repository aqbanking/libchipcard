/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id$
    begin       : Sun Jun 13 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CLIENT_TLV_H
#define CHIPCARD_CLIENT_TLV_H

#include <gwenhywfar/buffer.h>
#include <gwenhywfar/misc.h>


typedef struct LC_TLV LC_TLV;

GWEN_LIST_FUNCTION_DEFS(LC_TLV, LC_TLV)


LC_TLV *LC_TLV_new();
void LC_TLV_free(LC_TLV *tlv);

LC_TLV *LC_TLV_fromBuffer(GWEN_BUFFER *mbuf, int isBerTlv);

int LC_TLV_IsBerTlv(const LC_TLV *tlv);
unsigned int LC_TLV_GetTagType(const LC_TLV *tlv);
unsigned int LC_TLV_GetTagLength(const LC_TLV *tlv);
const void *LC_TLV_GetTagData(const LC_TLV *tlv);

int LC_TLV_IsContructed(const LC_TLV *tlv);
unsigned int LC_TLV_GetClass(const LC_TLV *tlv);
unsigned int LC_TLV_GetTagSize(const LC_TLV *tlv);




#endif /* CHIPCARD_CLIENT_TLV_H */
