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



#ifndef CHIPCARD_COMMON_DRIVERINFO_H
#define CHIPCARD_COMMON_DRIVERINFO_H

#include <gwenhywfar/stringlist.h>
#include <gwenhywfar/buffer.h>
#include <gwenhywfar/xml.h>
#include <gwenhywfar/db.h>


int LC_DriverInfo_FindFile(GWEN_STRINGLIST *slDirs,
                           GWEN_STRINGLIST *slNames,
                           GWEN_BUFFER *nbuf);


void LC_DriverInfo_SampleDirs(const char *dataDir, GWEN_STRINGLIST *sl);

GWEN_DB_NODE *LC_DriverInfo_DriverDbFromXml(GWEN_XMLNODE *node);
GWEN_DB_NODE *LC_DriverInfo_ReaderDbFromXml(GWEN_XMLNODE *node);

int LC_DriverInfo_SampleDrivers(GWEN_STRINGLIST *sl,
                                GWEN_DB_NODE *dbDrivers,
                                int availOnly);

int LC_DriverInfo_ReadDrivers(const char *dataDir,
                              GWEN_DB_NODE *dbDrivers,
                              int availOnly);

GWEN_TYPE_UINT32 LC_DriverInfo_ReaderFlagsFromDb(GWEN_DB_NODE *db,
                                                 const char *name);
GWEN_TYPE_UINT32 LC_DriverInfo_ReaderFlagsFromXml(GWEN_XMLNODE *node,
                                                  const char *name);





#endif
