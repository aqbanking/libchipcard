
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef BUILDING_LIBCHIPCARD2_DLL
# undef BUILDING_LIBCHIPCARD2_DLL
#endif

#define GWEN_EXTEND_WAITCALLBACK
#include <gwenhywfar/logger.h>
#include <gwenhywfar/net2.h>
#include <gwenhywfar/nl_ssl.h>
#include "cbtest.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "server_l.h"
#include "fullserver_l.h"
#include "pciscanner_l.h"
#include "usbrawscanner_l.h"
#include "usbttyscanner_l.h"
#ifdef USE_PCMCIA
# include "pcmciascanner_l.h"
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
    GWEN_NETLAYER_RESULT res;
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
    if (res==GWEN_NetLayerResult_Error) {
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
    GWEN_NETLAYER_RESULT res;

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
    if (res==GWEN_NetLayerResult_Error) {
      fprintf(stderr, "ERROR: Error while working (%d)\n", res);
      break;
    }
  }

  return 0;
}



int test8(int argc, char **argv) {
  LC_DEVMONITOR *devmon;
  LC_DEVSCANNER *devscan;
  int rv;
  LC_DEVICE_LIST *devlist;
  LC_DEVICE *dev;

  devmon=LC_DevMonitor_new();
  devscan=LC_PciScanner_new();
  LC_DevMonitor_AddScanner(devmon, devscan);

#ifdef USE_PCMCIA
  devscan=LC_PcmciaScanner_new();
  LC_DevMonitor_AddScanner(devmon, devscan);
#endif

  devscan=LC_UsbRawScanner_new();
  LC_DevMonitor_AddScanner(devmon, devscan);

  devscan=LC_UsbTtyScanner_new();
  LC_DevMonitor_AddScanner(devmon, devscan);

  rv=LC_DevMonitor_Scan(devmon);
  if (rv<0) {
    fprintf(stderr, "Error scanning\n");
  }
  else if (rv==1) {
    fprintf(stderr, "No changes.\n");
  }

  devlist=LC_DevMonitor_GetCurrentDevices(devmon);
  assert(devlist);

  fprintf(stderr, "List of devices:\n");
  dev=LC_Device_List_First(devlist);
  while(dev) {
    fprintf(stderr, "Device: %s %d %d 0x%04x 0x%04x\n",
            LC_Device_BusType_toString(LC_Device_GetBusType(dev)),
            LC_Device_GetBusId(dev),
            LC_Device_GetDeviceId(dev),
            LC_Device_GetVendorId(dev),
            LC_Device_GetProductId(dev));
    dev=LC_Device_List_Next(dev);
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
  GWEN_Logger_SetLevel(0, GWEN_LoggerLevelInfo);

  if (strcasecmp(argv[1], "test6")==0)
    return test6(argc, argv);
  else if (strcasecmp(argv[1], "test7")==0)
    return test7(argc, argv);
  else if (strcasecmp(argv[1], "test8")==0)
    return test8(argc, argv);
  else {
    fprintf(stderr, "Unknown command \"%s\"\n", argv[1]);
    return 1;
  }
}







