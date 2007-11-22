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



#ifndef CHIPCARD_SERVER2_SL_READER_P_H
#define CHIPCARD_SERVER2_SL_READER_P_H


#include "sl_reader_l.h"
#include <gwenhywfar/idlist.h>


typedef struct LCSL_READER LCSL_READER;
struct LCSL_READER {
  LCCO_CARD_LIST2 *insertedCards;
  LCCO_CARD_LIST2 *removedCards;
  uint32_t masterReaderId;
  uint32_t slaveReaderId;
  uint32_t flags;
};

static void GWENHYWFAR_CB LCSL_Reader_FreeData(void *bp, void *p);



#endif /* CHIPCARD_SERVER2_SL_READER_P_H */


