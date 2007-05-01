/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: cl_request_p.h 211 2006-09-07 23:57:04Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/



#ifndef CHIPCARD_SERVER2_SL_READER_L_H
#define CHIPCARD_SERVER2_SL_READER_L_H

#include "common/card.h"
#include "common/reader.h"
#include <gwenhywfar/misc.h>

#define LCSL_READER_FLAGS_REPORTED_UP   0x00000001
#define LCSL_READER_FLAGS_REPORTED_DOWN 0x00000002
#define LCSL_READER_FLAGS_STARTED       0x00000004
#define LCSL_READER_FLAGS_STOPPED       0x00000008


void LCSL_Reader_Extend(LCCO_READER *r);

LCCO_CARD *LCSL_Reader_GetNextInsertedCard(LCCO_READER *r);
void LCSL_Reader_AddInsertedCard(LCCO_READER *r, LCCO_CARD *card);

LCCO_CARD *LCSL_Reader_GetNextRemovedCard(LCCO_READER *r);
void LCSL_Reader_AddRemovedCard(LCCO_READER *r, LCCO_CARD *card);


GWEN_TYPE_UINT32 LCSL_Reader_GetFlags(const LCCO_READER *r);
void LCSL_Reader_SetFlags(LCCO_READER *r, GWEN_TYPE_UINT32 fl);
void LCSL_Reader_AddFlags(LCCO_READER *r, GWEN_TYPE_UINT32 fl);
void LCSL_Reader_DelFlags(LCCO_READER *r, GWEN_TYPE_UINT32 fl);

GWEN_TYPE_UINT32 LCSL_Reader_GetMasterReaderId(const LCCO_READER *r);
void LCSL_Reader_SetMasterReaderId(LCCO_READER *r, GWEN_TYPE_UINT32 i);

GWEN_TYPE_UINT32 LCSL_Reader_GetSlaveReaderId(const LCCO_READER *r);
void LCSL_Reader_SetSlaveReaderId(LCCO_READER *r, GWEN_TYPE_UINT32 i);

#endif /* CHIPCARD_SERVER2_SL_READER_P_H */


