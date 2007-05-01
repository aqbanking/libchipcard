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


#ifndef CHIPCARD_DRIVER_CTAPI_P_H
#define CHIPCARD_DRIVER_CTAPI_P_H

#include "driverctapi.h"

#include <gwenhywfar/libloader.h>
#include <chipcard3/server/driver/driver.h>


#define LCD_DRIVERCTAPI_DAD_CT   1

#define LCD_DRIVERCTAPI_SAD_HOST 2

#define DRIVER_CTAPI_ERROR_BAD_RESPONSE          1
#define DRIVER_CTAPI_ERROR_NO_SLOTS_CONNECTED    2
#define DRIVER_CTAPI_ERROR_NO_SLOTS_DISCONNECTED 3
#define DRIVER_CTAPI_ERROR_NO_SLOTS_AVAILABLE    4
#define DRIVER_CTAPI_ERROR_READER_INIT           5
#define DRIVER_CTAPI_ERROR_GENERIC               6


typedef char (*CTAPI_INIT_PTR) (unsigned short Ctn, unsigned short pn);
typedef char (*CTAPI_CLOSE_PTR)(unsigned short Ctn);
typedef char (*CTAPI_DATA_PTR) (unsigned short Ctn,
				unsigned char  *dad,
				unsigned char  *sad,
				unsigned short lc,
				unsigned char  *cmd,
				unsigned short *lr,
				unsigned char  *rsp);
typedef char (*KOBIL_INITNAME_PTR) (unsigned short Ctn, const char *dev);

typedef int (*RSCT_KEY_CB)(unsigned short ctn, void *user_data);
typedef char (*RSCT_SETKEYCB1_PTR)(unsigned short Ctn,
				   RSCT_KEY_CB cb,
				   void *user_data);
typedef char (*RSCT_INITNAME_PTR) (unsigned short Ctn, const char *dev);
typedef char (*RSCT_VERSION_PTR) (unsigned char *vmajor,
				  unsigned char *vminor,
				  unsigned char *vpatchlevel,
				  unsigned short *vbuild);




struct DRIVER_CTAPI {
  GWEN_LIBLOADER *libLoader;
  int stopDriver;
  int nextCtn;

  CTAPI_INIT_PTR initFn;
  CTAPI_CLOSE_PTR closeFn;
  CTAPI_DATA_PTR dataFn;
  KOBIL_INITNAME_PTR kobilInitNameFn;
  RSCT_SETKEYCB1_PTR rsctSetKeyCb1Fn;
  RSCT_INITNAME_PTR rsctInitNameFn;
  RSCT_VERSION_PTR rsctVersionFn;
};


void GWENHYWFAR_CB DriverCTAPI_freeData(void *bp, void *p);


int DriverCTAPI_TransformDAD(int i);
GWEN_TYPE_UINT32  DriverCTAPI_SendAPDU(LCD_DRIVER *d,
                                       int toReader,
                                       LCD_READER *r,
                                       LCD_SLOT *slot,
                                       const unsigned char *apdu,
                                       unsigned int apdulen,
                                       unsigned char *buffer,
                                       int *bufferlen);
GWEN_TYPE_UINT32  DriverCTAPI_ConnectSlot(LCD_DRIVER *d, LCD_SLOT *sl);
GWEN_TYPE_UINT32  DriverCTAPI_ConnectReader(LCD_DRIVER *d, LCD_READER *r);

GWEN_TYPE_UINT32  DriverCTAPI_DisconnectSlot(LCD_DRIVER *d, LCD_SLOT *sl);
GWEN_TYPE_UINT32  DriverCTAPI_DisconnectReader(LCD_DRIVER *d, LCD_READER *r);

GWEN_TYPE_UINT32  DriverCTAPI_ResetSlot(LCD_DRIVER *d, LCD_SLOT *sl);


GWEN_TYPE_UINT32  DriverCTAPI_ReaderStatus(LCD_DRIVER *d, LCD_READER *r);
const char *DriverCTAPI_GetErrorText(LCD_DRIVER *d, GWEN_TYPE_UINT32 err);

GWEN_TYPE_UINT32 DriverCTAPI_ReadReaderInfo(LCD_DRIVER *d, LCD_READER *r,
                                            GWEN_BUFFER *buf);


GWEN_TYPE_UINT32 DriverCTAPI_ReaderInfo(LCD_DRIVER *d, LCD_READER *r,
                                        GWEN_BUFFER *buf);
GWEN_TYPE_UINT32 DriverCTAPI_ReadReaderUnits(LCD_DRIVER *d, LCD_READER *r,
                                             GWEN_BUFFER *buf);

int DriverCTAPI_ExtendReader(LCD_DRIVER *d, LCD_READER *r);


int DriverCTAPI_KeyCallback1(unsigned short ctn, void *user_data);


#endif /* CHIPCARD_DRIVER_CTAPI_P_H */



