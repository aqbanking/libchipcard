

#define GWEN_EXTEND_WAITCALLBACK
#include <chipcard2-server/server/cardserver.h>
#include <chipcard2-server/common/usbmonitor.h>
#include <gwenhywfar/logger.h>
#include <gwenhywfar/net.h>
#include <gwenhywfar/nettransportssl.h>
#include "cbtest.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "server_l.h"
#include "fullserver_l.h"


int test1(int argc, char **argv) {
  LC_CARDSERVER *cardServer;
  GWEN_DB_NODE *db;
  GWEN_WAITCALLBACK *cb;

  cardServer=LC_CardServer_new(0);
  cb=TEST_Callback_new(GWEN_WAITCALLBACK_ID_SIMPLE_PROGRESS);
  if (GWEN_WaitCallback_Register(cb)) {
    fprintf(stderr, "Could not register callback\n");
    return 1;
  }

  //GWEN_WaitCallback_SetDistance(cb, 100);
  //GWEN_WaitCallback_Enter(GWEN_NETCONNECTION_CBID_IO);

  db=GWEN_DB_Group_new("config");
  if (GWEN_DB_ReadFile(db,
                       "chipcardd.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }
  if (LC_CardServer_ReadConfig(cardServer, db)) {
    fprintf(stderr, "\nERROR: Could not read config\n");
    return 1;
  }

  while(1) {
    GWEN_NETCONNECTION_WORKRESULT res;
    int rv;

    //fprintf(stderr, "Heartbeat\n");
    res=GWEN_Net_HeartBeat(2000);
    if (res==GWEN_NetConnectionWorkResult_Error) {
      fprintf(stderr, "\nERROR: Error while working (%d)\n", res);
    }

    //fprintf(stderr, "\nINFO: Changes\n");
    while(1) {
      rv=LC_CardServer_Work(cardServer);
      if (rv==-1) {
        fprintf(stderr,
                "\nERROR: Error while working on hardware (%d)\n", rv);
        break;
      }
      else if (rv==1)
        break;
      else {
        //fprintf(stderr, "INFO: loop done (something done)\n");
        //fprintf(stderr, "\nINFO: One loop with hardware done\n");
      }
    }
  } /* while */

  fprintf(stderr, "\nFinished.\n");

  LC_CardServer_free(cardServer);
  return 0;
}



int test2(int argc, char **argv) {
  LC_CARDSERVER *cardServer;
  GWEN_DB_NODE *db;

  GWEN_Logger_SetLevel(0, GWEN_LoggerLevelDebug);
  cardServer=LC_CardServer_new(0);

  db=GWEN_DB_Group_new("config");
  if (GWEN_DB_ReadFile(db,
                       "chipcardd.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }
  if (LC_CardServer_ReadConfig(cardServer, db)) {
    GWEN_DB_Group_free(db);
    fprintf(stderr, "\nERROR: Could not read config\n");
    return 1;
  }
  GWEN_DB_Group_free(db);
  LC_CardServer_free(cardServer);
  fprintf(stderr, "\nFinished.\n");
  return 0;
}


int test3(int argc, char **argv) {
  GWEN_DB_NODE *db;

  db=GWEN_DB_Group_new("certificate");
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "countryName", "DE");
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "commonName", "Martin Preuss");
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "organizationName", "Aquamaniac");
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "organizationalUnitName", "Libchipcard");
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "localityName", "Hamburg");
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "stateOrProvinceName", "Hamburg");

  if (GWEN_NetTransportSSL_GenerateCertAndKeyFile("test.crt",
                                                  1024,
                                                  123,
                                                  90,
                                                  db)) {
    fprintf(stderr, "Could not create certificate.\n");
    GWEN_DB_Group_free(db);
    return 1;
  }

  fprintf(stderr, "Certificate created.\n");
  GWEN_DB_Group_free(db);
  return 0;
}



int test4(int argc, char **argv) {
  LC_USBMONITOR *um;

  um=LC_USBMonitor_new();

  if (LC_USBMonitor_Scan(um)) {
    fprintf(stderr, "Could not read USB files.\n");
    return 2;
  }

  if (LC_USBMonitor_Scan(um)) {
    fprintf(stderr, "Could not read USB files.\n");
    return 2;
  }

  return 0;
}


#if 0
int test5(int argc, char **argv) {
  LC_PCIMONITOR *um;

  um=LC_PciMonitor_new();

  if (LC_PciMonitor_Scan(um)) {
    fprintf(stderr, "Could not read PCI files.\n");
    return 2;
  }

  if (LC_PciMonitor_Scan(um)) {
    fprintf(stderr, "Could not read PCI files.\n");
    return 2;
  }

  return 0;
}
#endif


int test6(int argc, char **argv) {
  LCS_SERVER *cs;
  LCDM_DEVICEMANAGER *dm;
  GWEN_DB_NODE *dbConfig;
  int rv;
  time_t t0;
  int isOn=0;

  dbConfig=GWEN_DB_Group_new("config");
  if (GWEN_DB_ReadFile(dbConfig,
                       "lstest.conf",
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "Could not load test config.\n");
    return 2;
  }
  cs=LCS_FullServer_new();
  rv=LCS_FullServer_Init(cs, dbConfig);
  if (rv) {
    fprintf(stderr, "Error initializing server.\n");
    return 3;
  }

  dm=LCS_Server_GetDeviceManager(cs);
  assert(dm);

  LCS_Server_BeginUseReaders(cs);
  isOn=1;

  t0=time(0);
  fprintf(stderr, "Starting server.\n");
  for (;;) {
    GWEN_NETCONNECTION_WORKRESULT res;
    time_t t1;

    for (;;) {
      fprintf(stderr, "Working (%d)...\n", isOn);
      rv=LCS_FullServer_Work(cs);
      if (rv!=0) {
        fprintf(stderr, "Change.\n");
      }
      else
        break;
    }
    res=GWEN_Net_HeartBeat(2000);
    if (res==GWEN_NetConnectionWorkResult_Error) {
      fprintf(stderr, "ERROR: Error while working (%d)\n", res);
      break;
    }
    t1=time(0);
    if (difftime(t1, t0)>30) {
      if (isOn) {
        fprintf(stderr, "================== Turning readers off.\n");
        LCS_Server_EndUseReaders(cs, 1);
        isOn=0;
      }
      else {
        fprintf(stderr, "================== Turning readers on.\n");
        LCS_Server_BeginUseReaders(cs);
        isOn=1;
      }
      t0=time(0);
    }
  }

  return 0;
}



int test7(int argc, char **argv) {
  LCS_SERVER *cs;
  LCDM_DEVICEMANAGER *dm;
  GWEN_DB_NODE *dbConfig;
  int rv;

  dbConfig=GWEN_DB_Group_new("config");
  if (GWEN_DB_ReadFile(dbConfig,
                       "lstest.conf",
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "Could not load test config.\n");
    return 2;
  }
  cs=LCS_FullServer_new();
  rv=LCS_FullServer_Init(cs, dbConfig);
  if (rv) {
    fprintf(stderr, "Error initializing server.\n");
    return 3;
  }

  dm=LCS_Server_GetDeviceManager(cs);
  assert(dm);

  fprintf(stderr, "Starting server.\n");
  for (;;) {
    GWEN_NETCONNECTION_WORKRESULT res;

    for (;;) {
      fprintf(stderr, "Working ...\n");
      rv=LCS_FullServer_Work(cs);
      if (rv!=0) {
	fprintf(stderr, "Change.\n");
      }
      else
	break;
    }
    res=GWEN_Net_HeartBeat(2000);
    if (res==GWEN_NetConnectionWorkResult_Error) {
      fprintf(stderr, "ERROR: Error while working (%d)\n", res);
      break;
    }
  }

  return 0;
}



int main(int argc, char **argv) {
  if (argc<2) {
    fprintf(stderr, "At least command name needed.\n");
    return 1;
  }
  //GWEN_Logger_Open(0, "server",
  //                 "server.log",
  //                 GWEN_LoggerTypeFile,
  //                 GWEN_LoggerFacilityUser);
  GWEN_Logger_SetLevel(0, GWEN_LoggerLevelDebug);

  if (strcasecmp(argv[1], "test1")==0)
    return test1(argc, argv);
  else if (strcasecmp(argv[1], "test2")==0)
    return test2(argc, argv);
  else if (strcasecmp(argv[1], "test3")==0)
    return test3(argc, argv);
  else if (strcasecmp(argv[1], "test4")==0)
    return test4(argc, argv);
#if 0
  else if (strcasecmp(argv[1], "test5")==0)
    return test5(argc, argv);
#endif
  else if (strcasecmp(argv[1], "test6")==0)
    return test6(argc, argv);
  else if (strcasecmp(argv[1], "test7")==0)
    return test7(argc, argv);
  else {
    fprintf(stderr, "Unknown command \"%s\"\n", argv[1]);
    return 1;
  }
}







