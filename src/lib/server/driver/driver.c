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


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "driver_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/nl_socket.h>
#include <gwenhywfar/nl_ssl.h>
#include <gwenhywfar/nl_http.h>
#include <gwenhywfar/net2.h>
#include <gwenhywfar/directory.h>

#include <chipcard/chipcard.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>

static GWEN_TYPE_UINT32 LCD_Driver__LastCardNum=0;


GWEN_INHERIT_FUNCTIONS(LCD_DRIVER)


void LCD_Driver_Usage(const char *prgName) {
  fprintf(stdout,
          "%s [OPTONS] \n"
          "[-v]               verbous\n"
          "[--logfile ARG]    name of the logfile\n"
          "[--logtype ARG]    log type\n"
          "[--loglevel ARG]   log level\n"
          "[-d ARG]           driver data folder\n"
          "-b ARG             server id\n"
          "[-u ARG]           customer id of this driver\n"
          "[-a ARG]           server IP address (or hostname)\n"
          "[-p ARG]           server TCP port\n"
          "-l ARG             name of the library driver file\n"
          "-i ARG             driver id for this session\n"
          "\n"
          "The following arguments are used in test mode only\n"
          "--test             enter test mode, check for a given reader\n"
          "-rp ARG            reader port\n"
          "-rs ARG            reader slots\n"
          "-rn ARG            reader name\n"
          "[-rt ARG]          reader type\n"
          "[-dt ARG]          driver type\n"
          "[-dp ARG           device path\n]"
          "--accept-all-certs accept all server certificates"
          , prgName
         );
}



LCD_DRIVER *LCD_Driver_new() {
  LCD_DRIVER *d;

  GWEN_NEW_OBJECT(LCD_DRIVER, d);
  GWEN_INHERIT_INIT(LCD_DRIVER, d);
  d->readers=LCD_Reader_List_new();

  return d;
}



void LCD_Driver_free(LCD_DRIVER *d) {
  if (d) {
    GWEN_INHERIT_FINI(LCD_DRIVER, d);
    LCD_Reader_List_free(d->readers);
    GWEN_IpcManager_free(d->ipcManager);
    free(d->logFile);
    free(d->readerLogFile);
    GWEN_FREE_OBJECT(d);
  }
}



LCD_DRIVER_CHECKARGS_RESULT LCD_Driver_CheckArgs(LCD_DRIVER *d,
                                                 int argc, char **argv) {
  int i;

  assert(d);

  d->verbous=0;
  d->rslots=1;
  d->testMode=0;
  d->rport=0;
  d->rname=0;
  d->rtype=0;
  d->dtype=0;
  d->logType=GWEN_LoggerTypeConsole;
  d->logFile=strdup("driver.log");
  d->logLevel=GWEN_LoggerLevelNotice;
  d->serverPort=LC_DEFAULT_PORT;
  d->typ="local";
  d->certFile=0;
  d->certDir=0;
  d->serverAddr=0;
  d->acceptAllCerts=0;
  d->devicePath=0;

  i=1;
  while (i<argc){
    if (strcmp(argv[i],"--logfile")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      free(d->logFile);
      d->logFile=strdup(argv[i]);
    }
    else if (strcmp(argv[i],"--logtype")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      d->logType=GWEN_Logger_Name2Logtype(argv[i]);
      if (d->logType==GWEN_LoggerTypeUnknown) {
        DBG_ERROR(0, "Unknown log type \"%s\"\n", argv[i]);
        return LCD_DriverCheckArgsResultError;
      }
    }
    else if (strcmp(argv[i],"--loglevel")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      d->logLevel=GWEN_Logger_Name2Level(argv[i]);
      if (d->logLevel==GWEN_LoggerLevelUnknown) {
        DBG_ERROR(0, "Unknown log level \"%s\"\n", argv[i]);
        return LCD_DriverCheckArgsResultError;
      }
    }
    else if (strcmp(argv[i], "--accept-all-certs")==0) {
      d->acceptAllCerts=1;
    }
    else if (strcmp(argv[i],"-d")==0) {
      i++;
      if (i>=argc)
        return -1;
      d->driverDataDir=argv[i];
    }
    else if (strcmp(argv[i],"-t")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      d->typ=argv[i];
    }
    else if (strcmp(argv[i],"-i")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      d->driverId=argv[i];
    }
    else if (strcmp(argv[i],"-a")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      d->serverAddr=argv[i];
    }
    else if (strcmp(argv[i],"-p")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      d->serverPort=atoi(argv[i]);
    }
    else if (strcmp(argv[i],"-l")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      d->libraryFile=argv[i];
    }
    else if (strcmp(argv[i],"-c")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      d->certFile=argv[i];
    }
    else if (strcmp(argv[i],"-C")==0) {
      i++;
      if (i>=argc)
        return LCD_DriverCheckArgsResultError;
      d->certDir=argv[i];
    }
    else if (strcmp(argv[i],"--test")==0) {
      d->testMode=1;
    }
    else if (strcmp(argv[i],"-rn")==0) {
      i++;
      if (i>=argc)
	return LCD_DriverCheckArgsResultError;
      d->rname=argv[i];
    }
    else if (strcmp(argv[i],"-rp")==0) {
      i++;
      if (i>=argc)
	return LCD_DriverCheckArgsResultError;
      d->rport=atoi(argv[i]);
    }
    else if (strcmp(argv[i],"-rs")==0) {
      i++;
      if (i>=argc)
	return LCD_DriverCheckArgsResultError;
      d->rslots=atoi(argv[i]);
    }
    else if (strcmp(argv[i],"-rt")==0) {
      i++;
      if (i>=argc)
	return LCD_DriverCheckArgsResultError;
      d->rtype=argv[i];
    }
    else if (strcmp(argv[i],"-dt")==0) {
      i++;
      if (i>=argc)
	return LCD_DriverCheckArgsResultError;
      d->dtype=argv[i];
    }
    else if (strcmp(argv[i],"-h")==0 || strcmp(argv[i],"--help")==0) {
      LCD_Driver_Usage(argv[0]);
      return LCD_DriverCheckArgsResultHelp;
    }
    else if (strcmp(argv[i],"-V")==0 || strcmp(argv[i],"--version")==0) {
      return LCD_DriverCheckArgsResultVersion;
    }
    else if (strcmp(argv[i],"-v")==0) {
      d->verbous=1;
    }
    else if (strcmp(argv[i],"-dp")==0) {
      i++;
      if (i>=argc)
	return LCD_DriverCheckArgsResultError;
      d->devicePath=argv[i];
    }
    else {
      DBG_ERROR(0, "Unknown argument \"%s\"", argv[i]);
      return LCD_DriverCheckArgsResultError;
    }
    i++;
  } /* while */

  /* check for missing arguments */
  if (!d->testMode) {
    if (!d->serverAddr) {
      DBG_ERROR(0, "Server address missing");
      return LCD_DriverCheckArgsResultError;
    }
    if (!d->driverId) {
      DBG_ERROR(0, "Driver id missing");
      return LCD_DriverCheckArgsResultError;
    }
  }
  else {
    if (d->rname==0)
      d->rname=strdup("testReader");
  }

  if (!d->libraryFile) {
    DBG_ERROR(0, "Name of driver library file missing");
    return LCD_DriverCheckArgsResultError;
  }

  if (d->logFile==0) {
    GWEN_BUFFER *mbuf;

    mbuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_AppendString(mbuf, "driver");
    GWEN_Buffer_AppendString(mbuf, ".log");
    d->logFile=strdup(GWEN_Buffer_GetStart(mbuf));
    GWEN_Buffer_free(mbuf);
  }

  if (d->logFile) {
    GWEN_BUFFER *mbuf;

    if (strstr(d->logFile, "@reader@")) {
      free(d->readerLogFile);
      d->readerLogFile=strdup(d->logFile);
      mbuf=GWEN_Buffer_new(0, 256, 0, 1);
      LCD_Driver_ReplaceVar(d->logFile, "reader", "driver", mbuf);
      free(d->logFile);
      d->logFile=strdup(GWEN_Buffer_GetStart(mbuf));
      GWEN_Buffer_free(mbuf);
    }
  }

  return 0;
}



int LCD_Driver_Init(LCD_DRIVER *d, int argc, char **argv) {
  LCD_DRIVER_CHECKARGS_RESULT res;

  res=LCD_Driver_CheckArgs(d, argc, argv);
  if (res!=LCD_DriverCheckArgsResultOk) {
    return -1;
  }

  if (!d->testMode) {
    GWEN_NETLAYER *nl;
    GWEN_NETLAYER *nlBase;
    GWEN_SOCKET *sk;
    GWEN_INETADDRESS *addr;
    GWEN_TYPE_UINT32 sid;
    GWEN_URL *url;

    url=GWEN_Url_fromString(d->serverAddr);
    if (!url) {
      DBG_ERROR(GWEN_LOGDOMAIN, "Bad URL: %s", d->serverAddr);
      return -1;
    }
  
    if (GWEN_Directory_GetPath(d->logFile,
                               GWEN_PATH_FLAGS_VARIABLE)) {
      DBG_ERROR(0, "Could not create log file for driver ");
      GWEN_Logger_Open(0, "driver",
                       0,
                       GWEN_LoggerTypeConsole,
                       GWEN_LoggerFacilityUser);
    }
    else {
      GWEN_Logger_Open(0, "driver",
                       d->logFile,
                       d->logType,
                       GWEN_LoggerFacilityUser);
    }
    GWEN_Logger_SetLevel(0, d->logLevel);
    DBG_NOTICE(0, "Starting driver \"%s\" with lowlevel \"%s\"",
               argv[0], d->libraryFile);
    if (d->driverDataDir) {
      if (chdir(d->driverDataDir)) {
        DBG_WARN(0, "chdir(%s): %s", d->driverDataDir, strerror(errno));
      }
    }

    d->ipcManager=GWEN_IpcManager_new();

    if (strcasecmp(d->typ, "local")==0) {
      /* HTTP over UDS */
      sk=GWEN_Socket_new(GWEN_SocketTypeUnix);
      addr=GWEN_InetAddr_new(GWEN_AddressFamilyUnix);
      GWEN_InetAddr_SetAddress(addr, d->serverAddr);
      nlBase=GWEN_NetLayerSocket_new(sk, 1);
      GWEN_NetLayer_SetPeerAddr(nlBase, addr);
    }
    else if (strcasecmp(d->typ, "public")==0) {
      /* HTTP over TCP */
      sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
      addr=GWEN_InetAddr_new(GWEN_AddressFamilyIP);
      GWEN_InetAddr_SetAddress(addr, d->serverAddr);
      GWEN_InetAddr_SetPort(addr, d->serverPort);
      nlBase=GWEN_NetLayerSocket_new(sk, 1);
      GWEN_NetLayer_SetPeerAddr(nlBase, addr);
    }
    else {
      sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
      addr=GWEN_InetAddr_new(GWEN_AddressFamilyIP);
      GWEN_InetAddr_SetAddress(addr, d->serverAddr);
      GWEN_InetAddr_SetPort(addr, d->serverPort);
      nlBase=GWEN_NetLayerSocket_new(sk, 1);
      GWEN_NetLayer_SetPeerAddr(nlBase, addr);

      if (strcasecmp(d->typ, "private")==0) {
	/* HTTP over SSL */
        nl=GWEN_NetLayerSsl_new(nlBase,
                                d->certDir,
                                0,
                                d->certFile,
                                0,
                                0);
        GWEN_NetLayer_free(nlBase);
        GWEN_NetLayerSsl_SetAskAddCertFn(nl, LCD_Driver_AskAddCert, (void*)d);
        nlBase=nl;
      }
      else if (strcasecmp(d->typ, "secure")==0) {
	/* HTTP over SSL with certificates */
        nl=GWEN_NetLayerSsl_new(nlBase,
                                d->certDir,
                                0,
                                d->certFile,
                                0,
                                1);
        GWEN_NetLayer_free(nlBase);
        nlBase=nl;
      }
      else {
	DBG_ERROR(0, "Unknown mode \"%s\"", d->typ);
        GWEN_InetAddr_free(addr);
        GWEN_Url_free(url);
        return -1;
      }
    }
    GWEN_InetAddr_free(addr);

    nl=GWEN_NetLayerHttp_new(nlBase);
    GWEN_NetLayer_free(nlBase);
    GWEN_NetLayerHttp_SetOutCommand(nl, "POST", url);
    GWEN_Url_free(url);

    sid=GWEN_IpcManager_AddClient(d->ipcManager,
                                  nl,
                                  LCD_DRIVER_MARK_DRIVER);
    if (sid==0) {
      DBG_ERROR(0, "Could not add IPC client");
      return -1;
    }

    d->ipcId=sid;
    DBG_INFO(0, "IPC stuff initialized");
  }

  return 0;
}



int LCD_Driver_IsTestMode(const LCD_DRIVER *d) {
  assert(d);
  return d->testMode;
}



int LCD_Driver_Test(LCD_DRIVER *d) {
  LCD_READER *r;
  GWEN_TYPE_UINT32 res;

  assert(d);
  if (!d->testMode) {
    DBG_ERROR(0, "Not in test mode");
    return -1;
  }

  r=LCD_Driver_CreateReader(d,
                            1,
                            d->rname,
                            d->rport,
                            d->devicePath,
                            d->rslots,
                            0);
  assert(r);
  fprintf(stdout, "Connecting reader...\n");
  res=LCD_Driver_ConnectReader(d, r);
  if (res!=0) {
    fprintf(stderr, "-> Could not connect reader (%s)\n",
	    LCD_Driver_GetErrorText(d, res));
    LCD_Reader_free(r);
    return -1;
  }
  fprintf(stdout, "-> Reader connected.\n");

  fprintf(stdout, "Disconnecting reader...\n");
  res=LCD_Driver_DisconnectReader(d, r);
  if (res!=0) {
    fprintf(stderr, "-> Could not disconnect reader (%s)\n",
	    LCD_Driver_GetErrorText(d, res));
    LCD_Reader_free(r);
    return -1;
  }
  fprintf(stdout, "-> Reader disconnected.\n");

  fprintf(stdout, "Reader is available.\n");
  LCD_Reader_free(r);
  return 0;
}



const char *LCD_Driver_GetDriverDataDir(const LCD_DRIVER *d){
  assert(d);
  return d->driverDataDir;
}



const char *LCD_Driver_GetLibraryFile(const LCD_DRIVER *d){
  assert(d);
  return d->libraryFile;
}



const char *LCD_Driver_GetDriverId(const LCD_DRIVER *d){
  assert(d);
  return d->driverId;
}



LCD_READER *LCD_Driver_FindReaderByName(const LCD_DRIVER *d, const char *name) {
  LCD_READER *r;

  assert(d);
  r=LCD_Reader_List_First(d->readers);
  while(r) {
    if (strcasecmp(name, LCD_Reader_GetName(r))==0)
      return r;
    r=LCD_Reader_List_Next(r);
  } /* while */
  return 0;
}



LCD_READER *LCD_Driver_FindReaderByPort(const LCD_DRIVER *d, int port) {
  LCD_READER *r;

  assert(d);
  r=LCD_Reader_List_First(d->readers);
  while(r) {
    if (port==LCD_Reader_GetPort(r))
      return r;
    r=LCD_Reader_List_Next(r);
  } /* while */
  return 0;
}



LCD_READER *LCD_Driver_FindReaderById(const LCD_DRIVER *d, GWEN_TYPE_UINT32 id){
  LCD_READER *r;

  assert(d);
  r=LCD_Reader_List_First(d->readers);
  while(r) {
    if (id==LCD_Reader_GetReaderId(r))
      return r;
    r=LCD_Reader_List_Next(r);
  } /* while */
  return 0;
}



LCD_READER *LCD_Driver_FindReaderByDriversId(const LCD_DRIVER *d,
                                             GWEN_TYPE_UINT32 id){
  LCD_READER *r;

  assert(d);
  r=LCD_Reader_List_First(d->readers);
  while(r) {
    if (id==LCD_Reader_GetDriversReaderId(r))
      return r;
    r=LCD_Reader_List_Next(r);
  } /* while */
  return 0;
}



void LCD_Driver_AddReader(LCD_DRIVER *d, LCD_READER *r){
  assert(d);
  assert(r);

  LCD_Reader_List_Add(r, d->readers);
}



void LCD_Driver_DelReader(LCD_DRIVER *d, LCD_READER *r){
  assert(d);
  assert(r);

  LCD_Reader_List_Del(r);
}



LCD_READER_LIST *LCD_Driver_GetReaders(const LCD_DRIVER *d){
  assert(d);
  return d->readers;
}



int LCD_Driver_ReplaceVar(const char *path,
                         const char *var,
                         const char *value,
                         GWEN_BUFFER *nbuf) {
  unsigned int vlen;

  vlen=strlen(var);

  while(*path) {
    int handled;

    handled=0;
    if (*path=='@') {
      if (strncmp(path+1, var, vlen)==0) {
        if (path[vlen+1]=='@') {
          /* found variable, replace it */
          GWEN_Buffer_AppendString(nbuf, value);
          path+=vlen+2;
          handled=1;
        }
      }
    }
    if (!handled) {
      GWEN_Buffer_AppendByte(nbuf, *path);
      path++;
    }
  } /* while */

  return 0;
}



GWEN_NL_SSL_ASKADDCERT_RESULT
LCD_Driver_AskAddCert(GWEN_NETLAYER *nl,
                      const GWEN_SSLCERTDESCR *cert,
                      void *user_data) {
  LCD_DRIVER *d;

  d=(LCD_DRIVER*)user_data;
  if (!d) {
    DBG_ERROR(0, "No user data in AskAddCert function");
    return GWEN_NetLayerSsl_AskAddCertResult_No;
  }

  if (d->acceptAllCerts)
    return GWEN_NetLayerSsl_AskAddCertResult_Tmp;
  return GWEN_NetLayerSsl_AskAddCertResult_No;
}







#include "d_handle.c"
#include "d_ipc.c"
#include "d_virtual.c"
#include "d_work.c"





