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


#ifndef CHIPCARD_SERVER_CARDSERVER_P_H
#define CHIPCARD_SERVER_CARDSERVER_P_H

#define LC_CARDSERVER_DEFAULT_DRIVERIDLETIMEOUT    30
#define LC_CARDSERVER_DEFAULT_DRIVERSTARTTIMEOUT   10
#define LC_CARDSERVER_DEFAULT_DRIVERSTOPTIMEOUT    10
#define LC_CARDSERVER_DEFAULT_DRIVERRSTTIME        10

#define LC_CARDSERVER_DEFAULT_READERIDLETIMEOUT    15
#define LC_CARDSERVER_DEFAULT_READERSTARTTIMEOUT   30
#define LC_CARDSERVER_DEFAULT_READERCMDTIMEOUT     60
#define LC_CARDSERVER_DEFAULT_READERRSTTIME        20

#define LC_CARDSERVER_DEFAULT_SERVICESTARTTIMEOUT  10
#define LC_CARDSERVER_DEFAULT_SERVICESTOPTIMEOUT   10
#define LC_CARDSERVER_DEFAULT_SERVICECMDTIMEOUT    60
#define LC_CARDSERVER_DEFAULT_SERVICEIDLETIMEOUT   30

#define LC_CARDSERVER_DEFAULT_USB_SCAN_INTERVAL    2
#define LC_CARDSERVER_DEFAULT_USBTTY_SCAN_INTERVAL 2

#define LC_CARDSERVER_DRVER_MAX_PENDING_COMMANDS   128

#define LC_CARDSERVER_MARK_SERVER 1
#define LC_CARDSERVER_MARK_DRIVER 2
#define LC_CARDSERVER_MARK_CLIENT 3

#include <gwenhywfar/nettransportssl.h>
#include <gwenhywfar/stringlist.h>

#include "cardserver_l.h"
#include <chipcard2-server/server/driver.h>
#include <chipcard2-server/server/reader.h>
#include <chipcard2-server/server/client.h>
#include <chipcard2-server/server/card.h>
#include <chipcard2-server/server/service.h>
#include <chipcard2-server/server/usbttymonitor.h>
#include <chipcard2-server/server/usbmonitor.h>

#include "commands/cardmgr_l.h"

#include <time.h>



struct LC_CARDSERVER {
  char *serverPrgName;
  char *serverPrgVersion;

  char *typeForDrivers;
  char *addrForDrivers;
  int portForDrivers;

  char *dataDir;

  GWEN_IPCMANAGER *ipcManager;
  LC_DRIVER_LIST *drivers;
  LC_READER_LIST *readers;
  LC_CLIENT_LIST *clients;

  LC_SERVICE_LIST *services;

  LC_CARD_LIST *activeCards;
  LC_CARD_LIST *freeCards;

  LC_REQUEST_LIST *requests;

  unsigned int driverStartTimeout;
  unsigned int driverStopTimeout;
  unsigned int readerStartTimeout;

  unsigned int driverIdleTimeout;
  unsigned int driverRestartTime;
  unsigned int readerIdleTimeout;
  unsigned int readerCommandTimeout;
  unsigned int readerRestartTime;

  unsigned int serviceStartTimeout;
  unsigned int serviceStopTimeout;
  unsigned int serviceIdleTimeout;
  unsigned int serviceCommandTimeout;

  unsigned int lastAutoReader;
  int disableAutoconf;
  time_t lastUsbScan;
  time_t lastUsbTtyScan;
  unsigned int usbScanInterval;
  unsigned int usbTtyScanInterval;

  GWEN_TYPE_UINT32 ipcId;

  LC_CARDMGR *cardManager;

  GWEN_DB_NODE *dbDrivers;
  GWEN_DB_NODE *dbConfigDrivers;
  LC_USBTTYMONITOR *usbTtyMonitor;
  LC_USBMONITOR *usbMonitor;

  int nextNewPort;
};


void LC_CardServer_CollectCommands(LC_CARDSERVER *cs);
int LC_CardServer_RemoveCardsForReader(LC_CARDSERVER *cs, LC_READER *r);
int LC_CardServer_RemoveCardsAt(LC_CARDSERVER *cs,
                                LC_READER *r,
                                unsigned int slotNum);

GWEN_TYPE_UINT32 LC_CardServer_GetFlags(GWEN_DB_NODE *db, const char *vname);

/** takes over cl */
void LC_CardServer_ClientDown(LC_CARDSERVER *cs, LC_CLIENT *cl);

int LC_CardServer_CheckCards(LC_CARDSERVER *cs);
int LC_CardServer_CheckClient(LC_CARDSERVER *cs, LC_CLIENT *cl);
int LC_CardServer_CheckClients(LC_CARDSERVER *cs);


int LC_CardServer_StartService(LC_CARDSERVER *cs, LC_SERVICE *as);
int LC_CardServer_StopService(LC_CARDSERVER *cs, LC_SERVICE *as);
int LC_CardServer_CheckService(LC_CARDSERVER *cs, LC_SERVICE *as);

int LC_CardServer_StartDriver(LC_CARDSERVER *cs, LC_DRIVER *d);
int LC_CardServer_StopDriver(LC_CARDSERVER *cs, LC_DRIVER *d);


int LC_CardServer_StartReader(LC_CARDSERVER *cs, LC_READER *r);
int LC_CardServer_StopReader(LC_CARDSERVER *cs, LC_READER *r);


/**
 * @return 0 if ok, -1 on error, and 1 if not yet answered
 */
int LC_CardServer_CheckRequest(LC_CARDSERVER *cs, LC_REQUEST *rq);

void LC_CardServer_RemoveRequest(LC_CARDSERVER *cs, LC_REQUEST *rq);


LC_CLIENT *LC_CardServer_FindClient(const LC_CARDSERVER *cs,
                                    GWEN_TYPE_UINT32 id);

LC_REQUEST *LC_CardServer_FindClientCardRequest(const LC_CARDSERVER *cs,
                                                const LC_CLIENT *cl,
                                                const LC_CARD *cd,
                                                const char *name);

LC_REQUEST *LC_CardServer_GetRequest(const LC_CARDSERVER *cs,
                                     GWEN_TYPE_UINT32 rid);

GWEN_NETTRANSPORTSSL_ASKADDCERT_RESULT
  LC_CardServer__AskAddCert(GWEN_NETTRANSPORT *tr,
                            GWEN_DB_NODE *cert);

int LC_CardServer__GetPassword(GWEN_NETTRANSPORT *tr,
                               char *buffer, int num,
                               int rwflag);

void LC_CardServer__Up(GWEN_NETCONNECTION *conn);

void LC_CardServer__Down(GWEN_NETCONNECTION *conn);




int LC_CardServer_SendErrorResponse(LC_CARDSERVER *cs,
                                    GWEN_TYPE_UINT32 rqid,
                                    int code,
                                    const char *text);
void LC_CardServer_SampleDirs(LC_CARDSERVER *cs,
                              GWEN_STRINGLIST *sl);


/** @name Driver Commands
 *
 */
/*@{*/
int LC_CardServer_HandleServiceReady(LC_CARDSERVER *cs,
                                     GWEN_TYPE_UINT32 rid,
                                     GWEN_DB_NODE *dbReq);
int LC_CardServer_HandleDriverReady(LC_CARDSERVER *cs,
                                    GWEN_TYPE_UINT32 rid,
                                    GWEN_DB_NODE *dbReq);
int LC_CardServer_HandleCardInserted(LC_CARDSERVER *cs,
                                     GWEN_TYPE_UINT32 rid,
                                     GWEN_DB_NODE *dbReq);
int LC_CardServer_HandleCardRemoved(LC_CARDSERVER *cs,
                                    GWEN_TYPE_UINT32 rid,
                                    GWEN_DB_NODE *dbReq);
int LC_CardServer_HandleReaderError(LC_CARDSERVER *cs,
                                    GWEN_TYPE_UINT32 rid,
                                    GWEN_DB_NODE *dbReq);

/*@}*/

/** @name Client Commands */
/*@{*/
int LC_CardServer_HandleClientReady(LC_CARDSERVER *cs,
                                    GWEN_TYPE_UINT32 rid,
                                    GWEN_DB_NODE *dbReq);

int LC_CardServer_HandleStartWait(LC_CARDSERVER *cs,
                                  GWEN_TYPE_UINT32 rid,
                                  GWEN_DB_NODE *dbReq);
int LC_CardServer_HandleStopWait(LC_CARDSERVER *cs,
                                 GWEN_TYPE_UINT32 rid,
                                 GWEN_DB_NODE *dbReq);

int LC_CardServer_HandleTakeCard(LC_CARDSERVER *cs,
                                 GWEN_TYPE_UINT32 rid,
                                 GWEN_DB_NODE *dbReq);
int LC_CardServer_HandleReleaseCard(LC_CARDSERVER *cs,
                                    GWEN_TYPE_UINT32 rid,
                                    GWEN_DB_NODE *dbReq);
int LC_CardServer_HandleCommandCard(LC_CARDSERVER *cs,
                                    GWEN_TYPE_UINT32 rid,
                                    GWEN_DB_NODE *dbReq);
int LC_CardServer_HandleExecCommand(LC_CARDSERVER *cs,
                                    GWEN_TYPE_UINT32 rid,
                                    GWEN_DB_NODE *dbReq);
int LC_CardServer_HandleSelectCard(LC_CARDSERVER *cs,
                                   GWEN_TYPE_UINT32 rid,
                                   GWEN_DB_NODE *dbReq);
int LC_CardServer_HandleSetNotify(LC_CARDSERVER *cs,
                                  GWEN_TYPE_UINT32 rid,
                                  GWEN_DB_NODE *dbReq);
int LC_CardServer_HandleGetDriverVar(LC_CARDSERVER *cs,
                                     GWEN_TYPE_UINT32 rid,
                                     GWEN_DB_NODE *dbReq);

/*@}*/



/** @name Service Commands */
/*@{*/
int LC_CardServer_HandleServiceOpen(LC_CARDSERVER *cs,
                                    GWEN_TYPE_UINT32 rid,
                                    GWEN_DB_NODE *dbReq);
int LC_CardServer_HandleServiceClose(LC_CARDSERVER *cs,
                                     GWEN_TYPE_UINT32 rid,
                                     GWEN_DB_NODE *dbReq);
int LC_CardServer_HandleServiceCommand(LC_CARDSERVER *cs,
                                       GWEN_TYPE_UINT32 rid,
                                       GWEN_DB_NODE *dbReq);

int LC_CardServer_CheckServiceRsp(LC_CARDSERVER *cs,
                                  LC_REQUEST *rq,
                                  GWEN_DB_NODE *dbServiceRsp,
                                  GWEN_DB_NODE *dbRsp);

int LC_CardServer_HandleServiceNotification(LC_CARDSERVER *cs,
                                            GWEN_TYPE_UINT32 rid,
                                            GWEN_DB_NODE *dbReq);


/*@}*/



int LC_CardServer_HandleNextCommand(LC_CARDSERVER *cs);


GWEN_TYPE_UINT32 LC_CardServer_SendResetCard(LC_CARDSERVER *cs,
                                             const LC_CARD *card);
GWEN_TYPE_UINT32 LC_CardServer_SendStartReader(LC_CARDSERVER *cs,
                                               const LC_READER *r);
GWEN_TYPE_UINT32 LC_CardServer_SendStopReader(LC_CARDSERVER *cs,
                                              const LC_READER *r);
GWEN_TYPE_UINT32 LC_CardServer_SendStopDriver(LC_CARDSERVER *cs,
                                              const LC_DRIVER *d);
GWEN_TYPE_UINT32 LC_CardServer_SendStopService(LC_CARDSERVER *cs,
                                               const LC_SERVICE *as);



int LC_CardServer_FindFile(GWEN_STRINGLIST *slDirs,
                           GWEN_STRINGLIST *slNames,
                           GWEN_BUFFER *nbuf);
GWEN_DB_NODE *LC_CardServer_DriverDbFromXml(GWEN_XMLNODE *node);
GWEN_DB_NODE *LC_CardServer_ReaderDbFromXml(GWEN_XMLNODE *node);

int LC_CardServer_SampleDrivers(LC_CARDSERVER *cs,
                                GWEN_STRINGLIST *sl,
                                GWEN_DB_NODE *dbDrivers,
                                int availOnly);

int LC_CardServer_USBDevice_Up(LC_CARDSERVER *cs, LC_USBDEVICE *ud);
int LC_CardServer_USBDevice_Down(LC_CARDSERVER *cs, LC_USBDEVICE *ud);

int LC_CardServer_USBTTYDevice_Up(LC_CARDSERVER *cs, LC_USBTTYDEVICE *ud);
int LC_CardServer_USBTTYDevice_Down(LC_CARDSERVER *cs, LC_USBTTYDEVICE *ud);


int LC_CardServer_Reader_Up(LC_CARDSERVER *cs, LC_READER *r);

int LC_CardServer_ScanUSB(LC_CARDSERVER *cs);


int LC_CardServer_ReplaceVar(const char *path,
                             const char *var,
                             const char *value,
                             GWEN_BUFFER *nbuf);

void LC_CardServer__SampleDirs(const char *dataDir, GWEN_STRINGLIST *sl);
int LC_CardServer__SampleDrivers(GWEN_STRINGLIST *sl,
                                 GWEN_DB_NODE *dbDrivers,
                                 int availOnly);

int LC_CardServer__USBDeviceToDB(LC_USBDEVICE *ud,
                                 GWEN_DB_NODE *dbDrivers,
                                 GWEN_DB_NODE *dbDriverStore,
                                 GWEN_DB_NODE *dbReaderStore);
int LC_CardServer__USBTTYDeviceToDB(LC_USBTTYDEVICE *ud,
                                    GWEN_DB_NODE *dbDrivers,
                                    GWEN_DB_NODE *dbDriverStore,
                                    GWEN_DB_NODE *dbReaderStore);



#endif /* CHIPCARD_SERVER_CARDSERVER_P_H */




