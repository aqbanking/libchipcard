/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: client_p.h 137 2005-11-03 13:07:50Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CLIENT_CLIENTPCSC_P_H
#define CHIPCARD_CLIENT_CLIENTPCSC_P_H

#include "clientpcsc_l.h"

#include "readerpcsc_l.h"



#ifdef OS_WIN32

#include <wtypes.h>

/* since MinGW still doesn not have support for winscard.dll we need
 * to use our own implementation (which simply loads the WINSCARD.DLL
 * dynamically)
 */


typedef
  LONG __stdcall  (*SCARD_ESTABLISHCONTEXT_FN)(DWORD dwScope,
                                               LPCVOID pvReserved1,
                                               LPCVOID pvReserved2,
                                               LPSCARDCONTEXT phContext);
typedef LONG __stdcall (*SCARD_RELEASECONTEXT_FN)(SCARDCONTEXT hContext);
typedef LONG __stdcall (*SCARD_LISTREADERS_FN)(SCARDCONTEXT hContext,
                                               LPCSTR mszGroups,
                                               LPSTR mszReaders,
                                               LPDWORD pcchReaders);
typedef LONG __stdcall (*SCARD_CONNECT_FN)(SCARDCONTEXT hContext,
                                           LPCSTR szReader,
                                           DWORD dwShareMode,
                                           DWORD dwPreferredProtocols,
                                           LPSCARDHANDLE phCard,
                                           LPDWORD pdwActiveProtocol);
typedef LONG __stdcall (*SCARD_DISCONNECT_FN)(SCARDHANDLE hCard,
                                              DWORD dwDisposition);
typedef LONG __stdcall (*SCARD_CONTROL_FN)(SCARDHANDLE hCard,
                                           DWORD dwControlCode,
                                           LPCVOID pbSendBuffer,
                                           DWORD cbSendLength,
                                           LPVOID pbRecvBuffer,
                                           DWORD cbRecvLength,
                                           LPDWORD lpBytesReturned);
typedef LONG __stdcall (*SCARD_TRANSMIT_FN)(SCARDHANDLE hCard,
                                            LPCSCARD_IO_REQUEST pioSendPci,
                                            LPCBYTE pbSendBuffer,
                                            DWORD cbSendLength,
                                            LPCSCARD_IO_REQUEST pioRecvPci,
                                            LPBYTE pbRecvBuffer,
                                            LPDWORD pcbRecvLength);
typedef LONG __stdcall (*SCARD_STATUS_FN)(SCARDHANDLE hCard,
                                          LPSTR szReaderName,
                                          LPDWORD pcchReaderLen,
                                          LPDWORD pdwState,
                                          LPDWORD pdwProtocol,
                                          LPBYTE pbAtr,
                                          LPDWORD pcbAtrLen);



static int MySCard_LoadLibrary();
static int MySCard_UnloadLibrary();


static 
LONG MySCardEstablishContext(DWORD dwScope,
                             LPCVOID pvReserved1,
                             LPCVOID pvReserved2,
                             LPSCARDCONTEXT phContext);
#define SCardEstablishContext MySCardEstablishContext

static
LONG MySCardReleaseContext(SCARDCONTEXT hContext);
#define SCardReleaseContext MySCardReleaseContext


static 
LONG MySCardConnectA(SCARDCONTEXT hContext,
                     LPCTSTR szReader,
                     DWORD dwShareMode,
                     DWORD dwPreferredProtocols,
                     LPSCARDHANDLE phCard,
                     LPDWORD pdwActiveProtocol);
#define MySCardConnect MySCardConnectA
#define SCardConnect MySCardConnectA


static 
LONG MySCardDisconnect(SCARDHANDLE hCard,
                       DWORD dwDisposition);
#define SCardDisconnect MySCardDisconnect


static 
LONG MySCardStatusA(SCARDHANDLE hCard,
                    LPTSTR mszReaderNames,
                    LPDWORD pcchReaderLen,
                    LPDWORD pdwState,
                    LPDWORD pdwProtocol,
                    LPBYTE pbAtr,
                    LPDWORD pcbAtrLen);
#define MySCardStatus MySCardStatusA
#define SCardStatus MySCardStatusA


static 
LONG MySCardControl(SCARDHANDLE hCard,
                    DWORD dwControlCode,
                    LPCVOID pbSendBuffer,
                    DWORD cbSendLength,
                    LPVOID pbRecvBuffer,
                    DWORD cbRecvLength,
                    LPDWORD lpBytesReturned);
#define SCardControl MySCardControl


static 
LONG MySCardTransmit(SCARDHANDLE hCard,
                     LPCSCARD_IO_REQUEST pioSendPci,
                     LPCBYTE pbSendBuffer,
                     DWORD cbSendLength,
                     LPSCARD_IO_REQUEST pioRecvPci,
                     LPBYTE pbRecvBuffer,
                     LPDWORD pcbRecvLength);
#define SCardTransmit MySCardTransmit

static 
LONG MySCardListReadersA(SCARDCONTEXT hContext,
                         LPCTSTR mszGroups,
                         LPTSTR mszReaders,
                         LPDWORD pcchReaders);
#define MySCardListReaders MySCardListReadersA
#define SCardListReaders MySCardListReadersA


#else

/* non-Windows */

LONG SCardEstablishContext(DWORD dwScope,
                           LPCVOID pvReserved1,
                           LPCVOID pvReserved2,
                           LPSCARDCONTEXT phContext);

LONG SCardReleaseContext(SCARDCONTEXT hContext);


LONG SCardConnect(SCARDCONTEXT hContext,
                  LPCTSTR szReader,
                  DWORD dwShareMode,
                  DWORD dwPreferredProtocols,
                  LPSCARDHANDLE phCard,
                  LPDWORD pdwActiveProtocol);


LONG SCardDisconnect(SCARDHANDLE hCard,
                     DWORD dwDisposition);


LONG SCardStatus(SCARDHANDLE hCard,
                 LPTSTR mszReaderNames,
                 LPDWORD pcchReaderLen,
                 LPDWORD pdwState,
                 LPDWORD pdwProtocol,
                 LPBYTE pbAtr,
                 LPDWORD pcbAtrLen);


LONG SCardControl(SCARDHANDLE hCard,
                  DWORD dwControlCode,
                  LPCVOID pbSendBuffer,
                  DWORD cbSendLength,
                  LPVOID pbRecvBuffer,
                  DWORD cbRecvLength,
                  LPDWORD lpBytesReturned);


LONG SCardTransmit(SCARDHANDLE hCard,
                   LPCSCARD_IO_REQUEST pioSendPci,
                   LPCBYTE pbSendBuffer,
                   DWORD cbSendLength,
                   LPSCARD_IO_REQUEST pioRecvPci,
                   LPBYTE pbRecvBuffer,
                   LPDWORD pcbRecvLength);

LONG SCardListReaders(SCARDCONTEXT hContext,
                      LPCTSTR mszGroups,
                      LPTSTR mszReaders,
                      LPDWORD pcchReaders);


#endif






typedef struct LC_CLIENT_PCSC LC_CLIENT_PCSC;
struct LC_CLIENT_PCSC {
  SCARDCONTEXT scardContext;
  LC_READER_PCSC_LIST *readers;
  LC_READER_PCSC *lastUsedReader;

  uint32_t lastCardId;

  LC_CLIENT_INIT_FN initFn;
  LC_CLIENT_FINI_FN finiFn;
};

static void GWENHYWFAR_CB LC_ClientPcsc_FreeData(void *bp, void *p);



static LC_CLIENT_RESULT CHIPCARD_CB LC_ClientPcsc_V_Init(LC_CLIENT *cl,
                                                         GWEN_DB_NODE *db);
static LC_CLIENT_RESULT CHIPCARD_CB LC_ClientPcsc_V_Fini(LC_CLIENT *cl);

static LC_CLIENT_RESULT CHIPCARD_CB LC_ClientPcsc_V_Start(LC_CLIENT *cl);
static LC_CLIENT_RESULT CHIPCARD_CB LC_ClientPcsc_V_Stop(LC_CLIENT *cl);

static LC_CLIENT_RESULT CHIPCARD_CB
  LC_ClientPcsc_V_GetNextCard(LC_CLIENT *cl,
                              LC_CARD **pCard,
                              int timeout);
static LC_CLIENT_RESULT CHIPCARD_CB
  LC_ClientPcsc_V_ReleaseCard(LC_CLIENT *cl,
                              LC_CARD *card);

static LC_CLIENT_RESULT CHIPCARD_CB
  LC_ClientPcsc_V_ExecApdu(LC_CLIENT *cl,
                           LC_CARD *card,
                           const char *apdu,
                           unsigned int len,
                           GWEN_BUFFER *rbuf,
                           LC_CLIENT_CMDTARGET t,
                           int timeout);


static LC_CLIENT_RESULT LC_ClientPcsc_ConnectReader(LC_CLIENT *cl,
						    LC_READER_PCSC *r);
static LC_READER_PCSC *LC_ClientPcsc_FindReader(LC_CLIENT *cl,
						const char *rname);
static LC_CLIENT_RESULT LC_ClientPcsc_ScanReaders(LC_CLIENT *cl);


#endif /* CHIPCARD_CLIENT_CLIENTPCSC_P_H */

