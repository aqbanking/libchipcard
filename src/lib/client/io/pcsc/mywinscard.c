


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include <windows.h>
#include <stdio.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>


static int initCount=0;
static HINSTANCE libHandle=0;
static SCARD_ESTABLISHCONTEXT_FN establishContextFn=0;
static SCARD_RELEASECONTEXT_FN releaseContextFn=0;
static SCARD_LISTREADERS_FN listReadersFn=0;
static SCARD_CONNECT_FN connectFn=0;
static SCARD_DISCONNECT_FN disconnectFn=0;
static SCARD_CONTROL_FN controlFn=0;
static SCARD_TRANSMIT_FN transmitFn=0;
static SCARD_STATUS_FN statusFn=0;



static int MySCard_LoadLibrary(const char *fname) {
  if (initCount==0) {
    void *fn;
  
    /*fprintf(stderr, "Loading library\n");*/
    libHandle=LoadLibrary(fname);
    if (!libHandle) {
      int werr;
      char *lpMsgBuf; /* from: http://msdn.microsoft.com/library/default.asp?url=/library/en-us/debug/base/formatmessage.asp */
  
      werr=GetLastError();
  
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    werr,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
                    (LPTSTR) &lpMsgBuf,
                    0,
                    NULL);
      /*fprintf(stderr, "Error loading DLL \"%s\": %d\n%d = %s\n",
              fname, werr, werr, lpMsgBuf);*/
      LocalFree(lpMsgBuf);
  
      if ( (werr == ERROR_DLL_NOT_FOUND) ||
           (werr == ERROR_FILE_NOT_FOUND) ||
           (werr == ERROR_MOD_NOT_FOUND) ) {
        /*fprintf(stderr, "File \"%s\" not found", fname);*/
        return -1;
      }
      /* TODO: Find the code for resolve errors */
      else
        return -1;
    }
  
    /* SCardEstablishContext */
    fn=GetProcAddress(libHandle, "SCardEstablishContext");
    if (fn==0) {
      FreeLibrary(libHandle);
      libHandle=0;
      return -1;
    }
    establishContextFn=fn;
  
    /* SCardReleaseContext */
    fn=GetProcAddress(libHandle, "SCardReleaseContext");
    if (fn==0) {
      FreeLibrary(libHandle);
      libHandle=0;
      return -1;
    }
    releaseContextFn=fn;
  
    /* SCardConnect */
    fn=GetProcAddress(libHandle, "SCardConnectA");
    if (fn==0) {
      FreeLibrary(libHandle);
      libHandle=0;
      return -1;
    }
    connectFn=fn;
  
    /* SCardDisconnect */
    fn=GetProcAddress(libHandle, "SCardDisconnect");
    if (fn==0) {
      FreeLibrary(libHandle);
      libHandle=0;
      return -1;
    }
    disconnectFn=fn;
  
    /* SCardStatus */
    fn=GetProcAddress(libHandle, "SCardStatusA");
    if (fn==0) {
      FreeLibrary(libHandle);
      libHandle=0;
      return -1;
    }
    statusFn=fn;
  
    /* SCardControl */
    fn=GetProcAddress(libHandle, "SCardControl");
    if (fn==0) {
      FreeLibrary(libHandle);
      libHandle=0;
      return -1;
    }
    controlFn=fn;
  
    /* SCardTransmit */
    fn=GetProcAddress(libHandle, "SCardTransmit");
    if (fn==0) {
      FreeLibrary(libHandle);
      libHandle=0;
      return -1;
    }
    transmitFn=fn;
  
    /* SCardListReaders */
    fn=GetProcAddress(libHandle, "SCardListReadersA");
    if (fn==0) {
      FreeLibrary(libHandle);
      libHandle=0;
      return -1;
    }
    listReadersFn=fn;
  }
  initCount++;

  return 0;
}



static int MySCard_UnloadLibrary() {
  if (initCount==1) {
    FreeLibrary(libHandle);
    libHandle=0;
  }
  initCount--;
  return 0;
}



LONG MySCardEstablishContext(DWORD dwScope,
                           LPCVOID pvReserved1,
                           LPCVOID pvReserved2,
                           LPSCARDCONTEXT phContext) {
  return establishContextFn(dwScope,
                            pvReserved1,
                            pvReserved2,
                            phContext);
}



LONG MySCardReleaseContext(SCARDCONTEXT hContext) {
  return releaseContextFn(hContext);
}



LONG MySCardConnectA(SCARDCONTEXT hContext,
                   LPCTSTR szReader,
                   DWORD dwShareMode,
                   DWORD dwPreferredProtocols,
                   LPSCARDHANDLE phCard,
                   LPDWORD pdwActiveProtocol) {
  return connectFn(hContext,
                   szReader,
                   dwShareMode,
                   dwPreferredProtocols,
                   phCard,
                   pdwActiveProtocol);
}



LONG MySCardDisconnect(SCARDHANDLE hCard,
                     DWORD dwDisposition) {
  return disconnectFn(hCard,
                      dwDisposition);
}



LONG MySCardStatusA(SCARDHANDLE hCard,
                  LPTSTR mszReaderNames,
                  LPDWORD pcchReaderLen,
                  LPDWORD pdwState,
                  LPDWORD pdwProtocol,
                  LPBYTE pbAtr,
                  LPDWORD pcbAtrLen) {
  return statusFn(hCard,
                  mszReaderNames,
                  pcchReaderLen,
                  pdwState,
                  pdwProtocol,
                  pbAtr,
                  pcbAtrLen);
}



LONG MySCardControl(SCARDHANDLE hCard,
                  DWORD dwControlCode,
                  LPCVOID pbSendBuffer,
                  DWORD cbSendLength,
                  LPVOID pbRecvBuffer,
                  DWORD cbRecvLength,
                  LPDWORD lpBytesReturned) {
  return controlFn(hCard,
                   dwControlCode,
                   pbSendBuffer,
                   cbSendLength,
                   pbRecvBuffer,
                   cbRecvLength,
                   lpBytesReturned);
}



LONG MySCardTransmit(SCARDHANDLE hCard,
                   LPCSCARD_IO_REQUEST pioSendPci,
                   LPCBYTE pbSendBuffer,
                   DWORD cbSendLength,
                   LPSCARD_IO_REQUEST pioRecvPci,
                   LPBYTE pbRecvBuffer,
                   LPDWORD pcbRecvLength) {
  return transmitFn(hCard,
                    pioSendPci,
                    pbSendBuffer,
                    cbSendLength,
                    pioRecvPci,
                    pbRecvBuffer,
                    pcbRecvLength);
}



LONG MySCardListReadersA(SCARDCONTEXT hContext,
                         LPCTSTR mszGroups,
                         LPTSTR mszReaders,
                         LPDWORD pcchReaders) {
  return listReadersFn(hContext,
                       mszGroups,
                       mszReaders,
                       pcchReaders);
}




