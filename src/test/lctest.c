

#define GWEN_EXTEND_WAITCALLBACK
#include <chipcard2/chipcard2.h>
#include <chipcard2-client/client/client.h>
#include <chipcard2-client/cards/processorcard.h>
#include <chipcard2-client/cards/ddvcard.h>
#include <chipcard2-client/cards/kvkcard.h>
#include <chipcard2-client/cards/starcos.h>
#include <chipcard2-client/cards/geldkarte.h>
#include <chipcard2-client/fs/fs.h>
#include <chipcard2-client/fs/fsfile.h>
#include <gwenhywfar/logger.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inetsocket.h>
#include "cbtest.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#ifdef OS_WIN32
# define sleep(x) GWEN_Socket_Select(0, 0, 0, (x)*1000)
#endif


int test1(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  GWEN_TYPE_UINT32 rqid;
  LC_CARD *card;
  GWEN_BUFFER *dbuf;
  GWEN_WAITCALLBACK *cb;
  LC_CLIENT_RESULT res;

  cb=TEST_Callback_new(GWEN_WAITCALLBACK_ID_SIMPLE_PROGRESS);
  if (GWEN_WaitCallback_Register(cb)) {
    fprintf(stderr, "Could not register callback\n");
    return 1;
  }

  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending StartWait\n");
  rqid=LC_Client_SendStartWait(cl, 0, 0);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  res=LC_Client_CheckStartWait(cl, rqid);
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  fprintf(stderr, "INFO: Taking card\n");
  rqid=LC_Client_SendTakeCard(cl, card);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  res=LC_Client_CheckTakeCard(cl, rqid);
  fprintf(stderr, "Response was %d\n", res);

  dbuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Buffer_AppendByte(dbuf, 0x00);
  GWEN_Buffer_AppendByte(dbuf, 0xa4);
  GWEN_Buffer_AppendByte(dbuf, 0x00);
  GWEN_Buffer_AppendByte(dbuf, 0x00);
  GWEN_Buffer_AppendByte(dbuf, 0x02);
  GWEN_Buffer_AppendByte(dbuf, 0x3f);
  GWEN_Buffer_AppendByte(dbuf, 0x00);
  GWEN_Buffer_AppendByte(dbuf, 0x00);

  fprintf(stderr,
          "HINT: Remove the card to check for changeCheck\n");

  sleep(5);

  fprintf(stderr, "INFO: Sending command\n");
  rqid=LC_Client_SendCommandCard(cl, card,
                                 GWEN_Buffer_GetStart(dbuf),
                                 GWEN_Buffer_GetUsedBytes(dbuf),
                                 LC_Client_CmdTargetCard);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  GWEN_Buffer_Reset(dbuf);
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  res=LC_Client_CheckCommandCard(cl, rqid, dbuf);
  fprintf(stderr, "Response was %d\n", res);
  GWEN_Buffer_Dump(dbuf, stderr, 2);

  rqid=LC_Client_SendStopWait(cl);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  res=LC_Client_CheckStopWait(cl, rqid);
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Sleeping...\n");
  sleep(5);

  LC_Client_free(cl);
  return 0;
}



int test2(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  GWEN_DB_NODE *dbCommand;
  GWEN_DB_NODE *dbCmdRsp;
  GWEN_TYPE_UINT32 rqid;
  LC_CARD *card;
  GWEN_WAITCALLBACK *cb;
  LC_CLIENT_RESULT res;
  time_t startTime;

  cb=TEST_Callback_new(GWEN_WAITCALLBACK_ID_SIMPLE_PROGRESS);
  if (GWEN_WaitCallback_Register(cb)) {
    fprintf(stderr, "Could not register callback\n");
    return 1;
  }

  //GWEN_WaitCallback_SetDistance(cb, 100);
  GWEN_WaitCallback_Enter(GWEN_WAITCALLBACK_ID_SIMPLE_PROGRESS);
  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending StartWait\n");
  rqid=LC_Client_SendStartWait(cl, 0, 0);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  res=LC_Client_CheckStartWait(cl, rqid);
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  fprintf(stderr, "INFO: Taking card\n");
  rqid=LC_Client_SendTakeCard(cl, card);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  res=LC_Client_CheckTakeCard(cl, rqid);
  fprintf(stderr, "Response was %d\n", res);

  startTime=time(0);
  rqid=LC_Client_SendSelectCardApp(cl, card, "ddv0", "ddv0");
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 10000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Time needed: %d secs\n",
          (int)difftime(time(0), startTime));

  res=LC_Client_CheckSelectCardApp(cl, rqid);
  fprintf(stderr, "Time needed now: %d secs\n",
          (int)difftime(time(0), startTime));
  fprintf(stderr, "Response was %d\n", res);

  //dbCommand=GWEN_DB_Group_new("SelectMF");

  dbCommand=GWEN_DB_Group_new("SelectDF");
  GWEN_DB_SetCharValue(dbCommand, GWEN_DB_FLAGS_DEFAULT,
                       "fname", "DF_BANKING");

  fprintf(stderr, "INFO: Sending command \"%s\"\n",
          GWEN_DB_GroupName(dbCommand));
  rqid=LC_Client_SendExecCommand(cl, card, dbCommand);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  dbCmdRsp=GWEN_DB_Group_new("Response");
  res=LC_Client_CheckExecCommand(cl, rqid, dbCmdRsp);
  fprintf(stderr, "Response was %d\n", res);
  GWEN_DB_Dump(dbCmdRsp, stderr, 2);



  dbCommand=GWEN_DB_Group_new("SelectEF");
  GWEN_DB_SetCharValue(dbCommand, GWEN_DB_FLAGS_DEFAULT,
                       "fname", "EF_BNK");

  fprintf(stderr, "INFO: Sending command \"%s\"\n",
          GWEN_DB_GroupName(dbCommand));
  rqid=LC_Client_SendExecCommand(cl, card, dbCommand);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  dbCmdRsp=GWEN_DB_Group_new("Response");
  res=LC_Client_CheckExecCommand(cl, rqid, dbCmdRsp);
  fprintf(stderr, "Response was %d\n", res);
  GWEN_DB_Dump(dbCmdRsp, stderr, 2);

  dbCommand=GWEN_DB_Group_new("ReadRecord");
  GWEN_DB_SetIntValue(dbCommand, GWEN_DB_FLAGS_DEFAULT,
                      "recNum", 1);

  fprintf(stderr, "INFO: Sending command \"%s\"\n",
          GWEN_DB_GroupName(dbCommand));
  rqid=LC_Client_SendExecCommand(cl, card, dbCommand);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  dbCmdRsp=GWEN_DB_Group_new("Response");
  res=LC_Client_CheckExecCommand(cl, rqid, dbCmdRsp);
  fprintf(stderr, "Response was %d\n", res);
  GWEN_DB_Dump(dbCmdRsp, stderr, 2);

  rqid=LC_Client_SendStopWait(cl);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  res=LC_Client_CheckStopWait(cl, rqid);
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Sleeping...\n");
  sleep(5);

  LC_Client_free(cl);
  return 0;
}



int test3(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  GWEN_TYPE_UINT32 rqid;
  LC_CARD *card;
  GWEN_WAITCALLBACK *cb;
  LC_CLIENT_RESULT res;
  time_t startTime;

  cb=TEST_Callback_new(GWEN_WAITCALLBACK_ID_SIMPLE_PROGRESS);
  if (GWEN_WaitCallback_Register(cb)) {
    fprintf(stderr, "Could not register callback\n");
    return 1;
  }

  //GWEN_WaitCallback_SetDistance(cb, 100);
  //GWEN_WaitCallback_Enter(GWEN_NETCONNECTION_CBID_IO);
  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending StartWait\n");
  rqid=LC_Client_SendStartWait(cl, 0, 0);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  res=LC_Client_CheckStartWait(cl, rqid);
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  startTime=time(0);
  fprintf(stderr, "INFO: Taking card\n");
  rqid=LC_Client_SendTakeCard(cl, card);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  res=LC_Client_CheckTakeCard(cl, rqid);
  fprintf(stderr, "Response was %d\n", res);
  fprintf(stderr, "Time needed: %d secs\n",
          (int)difftime(time(0), startTime));

  fprintf(stderr, "Sleeping...\n");
  sleep(5);

  LC_Client_free(cl);
  return 0;
}



int test4(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  GWEN_DB_NODE *dbCommand;
  GWEN_DB_NODE *dbCmdRsp;
  GWEN_TYPE_UINT32 rqid;
  LC_CARD *card;
  GWEN_WAITCALLBACK *cb;
  LC_CLIENT_RESULT res;
  time_t startTime;

  cb=TEST_Callback_new(GWEN_WAITCALLBACK_ID_SIMPLE_PROGRESS);
  if (GWEN_WaitCallback_Register(cb)) {
    fprintf(stderr, "Could not register callback\n");
    return 1;
  }

  //GWEN_WaitCallback_SetDistance(cb, 100);
  GWEN_WaitCallback_Enter(GWEN_WAITCALLBACK_ID_SIMPLE_PROGRESS);
  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending StartWait\n");
  rqid=LC_Client_SendStartWait(cl, 0, 0);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  res=LC_Client_CheckStartWait(cl, rqid);
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  fprintf(stderr, "INFO: Taking card\n");
  rqid=LC_Client_SendTakeCard(cl, card);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  res=LC_Client_CheckTakeCard(cl, rqid);
  fprintf(stderr, "Response was %d\n", res);

  startTime=time(0);
  rqid=LC_Client_SendSelectCardApp(cl, card, "ddv1", "ddv1");
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 10000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Time needed: %d secs\n",
          (int)difftime(time(0), startTime));

  res=LC_Client_CheckSelectCardApp(cl, rqid);
  fprintf(stderr, "Time needed now: %d secs\n",
          (int)difftime(time(0), startTime));
  fprintf(stderr, "Response was %d\n", res);

  //dbCommand=GWEN_DB_Group_new("SelectMF");

  dbCommand=GWEN_DB_Group_new("SelectDF");
  GWEN_DB_SetCharValue(dbCommand, GWEN_DB_FLAGS_DEFAULT,
                       "fname", "DF_BANKING");

  fprintf(stderr, "INFO: Sending command \"%s\"\n",
          GWEN_DB_GroupName(dbCommand));
  rqid=LC_Client_SendExecCommand(cl, card, dbCommand);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  dbCmdRsp=GWEN_DB_Group_new("Response");
  res=LC_Client_CheckExecCommand(cl, rqid, dbCmdRsp);
  fprintf(stderr, "Response was %d\n", res);
  GWEN_DB_Dump(dbCmdRsp, stderr, 2);



  //dbCommand=GWEN_DB_Group_new("SecureVerifyPin");
  dbCommand=GWEN_DB_Group_new("VerifyPin");
  GWEN_DB_SetCharValue(dbCommand, GWEN_DB_FLAGS_DEFAULT,
                       "pin", "12345");

  fprintf(stderr, "INFO: Sending command \"%s\"\n",
          GWEN_DB_GroupName(dbCommand));
  rqid=LC_Client_SendExecCommand(cl, card, dbCommand);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  dbCmdRsp=GWEN_DB_Group_new("Response");
  res=LC_Client_CheckExecCommand(cl, rqid, dbCmdRsp);
  fprintf(stderr, "Response was %d\n", res);
  GWEN_DB_Dump(dbCmdRsp, stderr, 2);

  rqid=LC_Client_SendStopWait(cl);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  res=LC_Client_CheckStopWait(cl, rqid);
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Sleeping...\n");
  sleep(5);

  LC_Client_free(cl);
  return 0;
}



int test5(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  GWEN_DB_NODE *dbCommand;
  GWEN_DB_NODE *dbCmdRsp;
  GWEN_TYPE_UINT32 rqid;
  LC_CARD *card;
  LC_CLIENT_RESULT res;
  time_t startTime;

  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending StartWait\n");
  rqid=LC_Client_SendStartWait(cl, 0, 0);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  res=LC_Client_CheckStartWait(cl, rqid);
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  fprintf(stderr, "INFO: Taking card\n");
  rqid=LC_Client_SendTakeCard(cl, card);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  res=LC_Client_CheckTakeCard(cl, rqid);
  fprintf(stderr, "Response was %d\n", res);

  startTime=time(0);
  rqid=LC_Client_SendSelectCardApp(cl, card, "ddv1", "ddv1");
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 10000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Time needed: %d secs\n",
          (int)difftime(time(0), startTime));

  res=LC_Client_CheckSelectCardApp(cl, rqid);
  fprintf(stderr, "Time needed now: %d secs\n",
          (int)difftime(time(0), startTime));
  fprintf(stderr, "Response was %d\n", res);

  //dbCommand=GWEN_DB_Group_new("SelectMF");

  dbCommand=GWEN_DB_Group_new("SelectDF");
  GWEN_DB_SetCharValue(dbCommand, GWEN_DB_FLAGS_DEFAULT,
                       "fname", "DF_BANKING_20");

  fprintf(stderr, "INFO: Sending command \"%s\"\n",
          GWEN_DB_GroupName(dbCommand));
  rqid=LC_Client_SendExecCommand(cl, card, dbCommand);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  dbCmdRsp=GWEN_DB_Group_new("Response");
  res=LC_Client_CheckExecCommand(cl, rqid, dbCmdRsp);
  fprintf(stderr, "Response was %d\n", res);
  GWEN_DB_Dump(dbCmdRsp, stderr, 2);



  dbCommand=GWEN_DB_Group_new("SelectEF");
  GWEN_DB_SetCharValue(dbCommand, GWEN_DB_FLAGS_DEFAULT,
                       "fname", "EF_FBZ");

  fprintf(stderr, "INFO: Sending command \"%s\"\n",
          GWEN_DB_GroupName(dbCommand));
  rqid=LC_Client_SendExecCommand(cl, card, dbCommand);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  dbCmdRsp=GWEN_DB_Group_new("Response");
  res=LC_Client_CheckExecCommand(cl, rqid, dbCmdRsp);
  fprintf(stderr, "Response was %d\n", res);
  GWEN_DB_Dump(dbCmdRsp, stderr, 2);

  dbCommand=GWEN_DB_Group_new("ReadRecord");
  GWEN_DB_SetIntValue(dbCommand, GWEN_DB_FLAGS_DEFAULT,
                      "recNum", 1);

  fprintf(stderr, "INFO: Sending command \"%s\"\n",
          GWEN_DB_GroupName(dbCommand));
  rqid=LC_Client_SendExecCommand(cl, card, dbCommand);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  dbCmdRsp=GWEN_DB_Group_new("Response");
  res=LC_Client_CheckExecCommand(cl, rqid, dbCmdRsp);
  fprintf(stderr, "Response was %d\n", res);
  GWEN_DB_Dump(dbCmdRsp, stderr, 2);

  rqid=LC_Client_SendStopWait(cl);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  res=LC_Client_CheckStopWait(cl, rqid);
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Sleeping...\n");
  sleep(5);

  LC_Client_free(cl);
  return 0;
}



int test6(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  GWEN_DB_NODE *dbCommand;
  GWEN_DB_NODE *dbCmdRsp;
  GWEN_TYPE_UINT32 rqid;
  LC_CARD *card;
  LC_CLIENT_RESULT res;
  time_t startTime;

  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending StartWait\n");
  rqid=LC_Client_SendStartWait(cl, 0, 0);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  res=LC_Client_CheckStartWait(cl, rqid);
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  fprintf(stderr, "INFO: Taking card\n");
  rqid=LC_Client_SendTakeCard(cl, card);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  res=LC_Client_CheckTakeCard(cl, rqid);
  fprintf(stderr, "Response was %d\n", res);

  startTime=time(0);
  rqid=LC_Client_SendSelectCardApp(cl, card, "ddv1", "ddv1");
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 10000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Time needed: %d secs\n",
          (int)difftime(time(0), startTime));

  res=LC_Client_CheckSelectCardApp(cl, rqid);
  fprintf(stderr, "Time needed now: %d secs\n",
          (int)difftime(time(0), startTime));
  fprintf(stderr, "Response was %d\n", res);

  //dbCommand=GWEN_DB_Group_new("SelectMF");

  dbCommand=GWEN_DB_Group_new("SelectEF");
  GWEN_DB_SetCharValue(dbCommand, GWEN_DB_FLAGS_DEFAULT,
                       "fname", "EF_ID");

  fprintf(stderr, "INFO: Sending command \"%s\"\n",
          GWEN_DB_GroupName(dbCommand));
  rqid=LC_Client_SendExecCommand(cl, card, dbCommand);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  dbCmdRsp=GWEN_DB_Group_new("Response");
  res=LC_Client_CheckExecCommand(cl, rqid, dbCmdRsp);
  fprintf(stderr, "Response was %d\n", res);
  GWEN_DB_Dump(dbCmdRsp, stderr, 2);

  dbCommand=GWEN_DB_Group_new("ReadRecord");
  GWEN_DB_SetIntValue(dbCommand, GWEN_DB_FLAGS_DEFAULT,
                      "recNum", 1);

  fprintf(stderr, "INFO: Sending command \"%s\"\n",
          GWEN_DB_GroupName(dbCommand));
  rqid=LC_Client_SendExecCommand(cl, card, dbCommand);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  dbCmdRsp=GWEN_DB_Group_new("Response");
  res=LC_Client_CheckExecCommand(cl, rqid, dbCmdRsp);
  fprintf(stderr, "Response was %d\n", res);
  GWEN_DB_Dump(dbCmdRsp, stderr, 2);

  rqid=LC_Client_SendStopWait(cl);
  if (rqid==0) {
    fprintf(stderr, "ERROR: Could not send request.\n");
    LC_Client_free(cl);
    return 2;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, 30000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  res=LC_Client_CheckStopWait(cl, rqid);
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Sleeping...\n");
  sleep(5);

  LC_Client_free(cl);
  return 0;
}



int test7(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  GWEN_DB_NODE *dbRecord;
  LC_CARD *card;
  LC_CLIENT_RESULT res;
  GWEN_BUFFER *mbuf;

  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending StartWait\n");
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  fprintf(stderr, "INFO: Opening card\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Stopping wait\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  res=LC_Card_SelectCardAndApp(card, "ddv1", "ddv1");
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Selecting EF...\n");
  res=LC_ProcessorCard_SelectEF(card, "EF_ID");
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Reading record...\n");
  mbuf=GWEN_Buffer_new(0, 256, 0, 1);
  res=LC_Card_IsoReadRecord(card, LC_CARD_ISO_FLAGS_RECSEL_GIVEN,
                            1, mbuf);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);
  GWEN_Text_DumpString(GWEN_Buffer_GetStart(mbuf),
                       GWEN_Buffer_GetUsedBytes(mbuf), stderr, 2);
  GWEN_Buffer_Rewind(mbuf);

  dbRecord=GWEN_DB_Group_new("record");
  if (LC_Card_ParseRecord(card, 1, mbuf, dbRecord)) {
    fprintf(stderr, "Error parsing record.\n");
  }
  else {
    fprintf(stderr, "Parsed record is:\n");
    GWEN_DB_Dump(dbRecord, stderr, 2);
  }

  GWEN_Buffer_Reset(mbuf);
  if (LC_Card_CreateRecord(card, 1, mbuf, dbRecord)) {
    fprintf(stderr, "Error creating record.\n");
  }
  else {
    fprintf(stderr, "Created record is:\n");
    GWEN_Buffer_Dump(mbuf, stderr, 2);
  }

  fprintf(stderr, "INFO: Closing card\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Sleeping...\n");
  sleep(5);

  LC_Client_free(cl);
  return 0;
}



int test8(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  LC_CARD *card;
  LC_CLIENT_RESULT res;

  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending StartWait\n");
  GWEN_Logger_SetLevel(LC_LOGDOMAIN, GWEN_LoggerLevelNotice);
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  while(1) {
    fprintf(stderr, "INFO: Waiting for card\n");
    card=LC_Client_WaitForNextCard(cl, 60);
    if (!card) {
      fprintf(stderr, "ERROR: No card found.\n");
    }
    else {
      fprintf(stderr, "INFO: We got this card:\n");
      LC_Card_Dump(card, stderr, 2);
    }
  }

  LC_Client_free(cl);
  return 0;
}



int test10(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  GWEN_DB_NODE *dbRecord;
  LC_CARD *card;
  LC_CLIENT_RESULT res;
  GWEN_BUFFER *mbuf;

  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending StartWait\n");
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  if (LC_DDVCard_ExtendCard(card)) {
    fprintf(stderr, "Could not extend card as DDV card\n");
    return 2;
  }

  fprintf(stderr, "INFO: Opening card\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Stopping wait\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Selecting EF...\n");
  res=LC_ProcessorCard_SelectEF(card, "EF_BNK");
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Reading record...\n");
  mbuf=GWEN_Buffer_new(0, 256, 0, 1);
  res=LC_Card_IsoReadRecord(card, LC_CARD_ISO_FLAGS_RECSEL_GIVEN,
                            1, mbuf);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);
  GWEN_Text_DumpString(GWEN_Buffer_GetStart(mbuf),
                       GWEN_Buffer_GetUsedBytes(mbuf), stderr, 2);
  GWEN_Buffer_Rewind(mbuf);

  dbRecord=GWEN_DB_Group_new("record");
  if (LC_Card_ParseRecord(card, 1, mbuf, dbRecord)) {
    fprintf(stderr, "Error parsing record.\n");
  }
  else {
    fprintf(stderr, "Parsed record is:\n");
    GWEN_DB_Dump(dbRecord, stderr, 2);
  }

  GWEN_Buffer_Reset(mbuf);
  if (LC_Card_CreateRecord(card, 1, mbuf, dbRecord)) {
    fprintf(stderr, "Error creating record.\n");
  }
  else {
    fprintf(stderr, "Created record is:\n");
    GWEN_Buffer_Dump(mbuf, stderr, 2);
  }

  fprintf(stderr, "INFO: Closing card\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Sleeping...\n");
  sleep(5);

  LC_Client_free(cl);
  return 0;
}



int test11(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  LC_CARD *card;
  LC_CLIENT_RESULT res;

  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending StartWait\n");
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  if (LC_DDVCard_ExtendCard(card)) {
    fprintf(stderr, "Could not extend card as DDV card\n");
    return 2;
  }

  fprintf(stderr, "INFO: Opening card\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Stopping wait\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Verifying PIN...\n");
  res=LC_DDVCard_SecureVerifyPin(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Closing card\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  LC_Client_free(cl);
  return 0;
}



int test12(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  GWEN_DB_NODE *dbRecord;
  LC_CARD *card;
  LC_CLIENT_RESULT res;

  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending StartWait\n");
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  if (LC_DDVCard_ExtendCard(card)) {
    fprintf(stderr, "Could not extend card as DDV card\n");
    return 2;
  }

  fprintf(stderr, "INFO: Opening card\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Stopping wait\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Reading institute data\n");
  dbRecord=GWEN_DB_Group_new("record");
  res=LC_DDVCard_ReadInstituteData(card, 1, dbRecord);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);
  GWEN_DB_Dump(dbRecord, stderr, 2);

  fprintf(stderr, "INFO: Closing card\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Sleeping...\n");
  sleep(5);

  LC_Client_free(cl);
  return 0;
}



int test13(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  GWEN_DB_NODE *dbRecord;
  LC_CARD *card;
  LC_CLIENT_RESULT res;

  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending StartWait\n");
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  if (LC_KVKCard_ExtendCard(card)) {
    fprintf(stderr, "Could not extend card as KVK card\n");
    return 2;
  }

  fprintf(stderr, "INFO: Opening card\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Stopping wait\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  dbRecord=LC_KVKCard_GetCardData(card);
  if (dbRecord) {
    fprintf(stderr, "Card data is:\n");
    GWEN_DB_Dump(dbRecord, stderr, 2);
  }
  else {
    fprintf(stderr, "ERROR: No card data\n");
    return 2;
  }

  fprintf(stderr, "INFO: Closing card\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Sleeping...\n");
  sleep(5);

  LC_Client_free(cl);
  return 0;
}



int test14(int argc, char **argv) {
  return 0;
}



int test15(int argc, char **argv) {
  return 0;
}



int test16(int argc, char **argv) {
  return 0;
}



int test17(int argc, char **argv) {
  return 0;
}



int test18(int argc, char **argv) {
  return 0;
}



int test19(int argc, char **argv) {
  return 0;
}



int test20(int argc, char **argv) {
  return 0;
}



int test21(int argc, char **argv) {
  return 0;
}



int test22(int argc, char **argv) {
  return 0;
}



int test23(int argc, char **argv) {
  return 0;
}



int test24(int argc, char **argv) {
  return 0;
}



int test25(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  GWEN_DB_NODE *dbRecord;
  LC_CARD *card;
  LC_CLIENT_RESULT res;
  GWEN_BUFFER *mbuf;

  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending StartWait\n");
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  if (LC_GeldKarte_ExtendCard(card)) {
    fprintf(stderr, "Could not extend card as Geldkarte\n");
    return 2;
  }

  fprintf(stderr, "INFO: Opening card\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Stopping wait\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Selecting EF...\n");
  res=LC_ProcessorCard_SelectEF(card, "EF_BLOG");
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Reading record...\n");
  mbuf=GWEN_Buffer_new(0, 256, 0, 1);
  res=LC_Card_IsoReadRecord(card, LC_CARD_ISO_FLAGS_RECSEL_GIVEN,
                            1, mbuf);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);
  GWEN_Text_DumpString(GWEN_Buffer_GetStart(mbuf),
                       GWEN_Buffer_GetUsedBytes(mbuf), stderr, 2);
  GWEN_Buffer_Rewind(mbuf);

  dbRecord=GWEN_DB_Group_new("record");
  if (LC_Card_ParseRecord(card, 1, mbuf, dbRecord)) {
    fprintf(stderr, "Error parsing record.\n");
  }
  else {
    fprintf(stderr, "Parsed record is:\n");
    GWEN_DB_Dump(dbRecord, stderr, 2);
  }

  fprintf(stderr, "INFO: Closing card\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  LC_Client_free(cl);
  return 0;
}



int test26(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  LC_CARD *card;
  LC_CLIENT_RESULT res;
  LC_GELDKARTE_BLOG_LIST2 *bll;

  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending StartWait\n");
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  if (LC_GeldKarte_ExtendCard(card)) {
    fprintf(stderr, "Could not extend card as Geldkarte\n");
    return 2;
  }

  fprintf(stderr, "INFO: Opening card\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Card is:\n");
  GWEN_DB_Dump(LC_GeldKarte_GetCardDataAsDb(card), stderr, 2);

  fprintf(stderr, "Account is:\n");
  GWEN_DB_Dump(LC_GeldKarte_GetAccountDataAsDb(card), stderr, 2);

  fprintf(stderr, "Stopping wait\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Reading blogs:\n");
  bll=LC_GeldKarte_BLog_List2_new();
  res=LC_GeldKarte_ReadBLogs(card, bll);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Closing card\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  LC_Client_free(cl);
  return 0;
}



int test27(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  LC_CARD *card;
  LC_CLIENT_RESULT res;

  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending SetNotify\n");
  res=LC_Client_SetNotify(cl,
                          LC_NOTIFY_FLAGS_READER_START|
                          LC_NOTIFY_FLAGS_READER_UP|
                          LC_NOTIFY_FLAGS_READER_DOWN|
                          LC_NOTIFY_FLAGS_READER_ERROR);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Sending StartWait\n");
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  fprintf(stderr, "Stopping wait\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  LC_Client_free(cl);
  return 0;
}



int test28(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  LC_CLIENT_RESULT res;

  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending SetNotify\n");
  res=LC_Client_SetNotify(cl,
                          LC_NOTIFY_FLAGS_READER_START|
                          LC_NOTIFY_FLAGS_READER_UP|
                          LC_NOTIFY_FLAGS_READER_DOWN|
                          LC_NOTIFY_FLAGS_READER_ERROR |
                          LC_NOTIFY_FLAGS_DRIVER_START|
                          LC_NOTIFY_FLAGS_DRIVER_UP|
                          LC_NOTIFY_FLAGS_DRIVER_DOWN|
                          LC_NOTIFY_FLAGS_DRIVER_ERROR);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  while(1) {
    res=LC_Client_Work_Wait(cl, 30);
    if (res==LC_Client_ResultOk) {
    }
    else if (res==LC_Client_ResultWait) {
      fprintf(stderr, "Timeout, retrying.\n");
    }
    else {
      fprintf(stderr, "Error (%d)\n", res);
      break;
    }
  }

  LC_Client_free(cl);
  return 0;
}



int test29(int argc, char **argv) {
  return 0;
}



int test30(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  LC_CARD *card;
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbData;
  int maxErrors;
  int currentErrors;

  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending StartWait\n");
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  if (LC_Starcos_ExtendCard(card)) {
    fprintf(stderr, "Could not extend card as STARCOS card\n");
    return 2;
  }

  fprintf(stderr, "INFO: Opening card\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  dbData=LC_Starcos_GetCardDataAsDb(card);
  GWEN_DB_Dump(dbData, stderr, 2);

  fprintf(stderr, "Stopping wait\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Getting pin status...\n");
  res=LC_Starcos_GetPinStatus(card, 0x90, &maxErrors, &currentErrors);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Pin Status: MaxErrors=%d, CurrentErrors=%d\n",
          maxErrors, currentErrors);

  fprintf(stderr, "INFO: Closing card\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Sleeping...\n");
  sleep(5);

  LC_Client_free(cl);
  return 0;
}



int _check31(LC_CLIENT *cl) {
  LC_CARD *card;
  LC_CLIENT_RESULT res;

  fprintf(stderr, "INFO: Sending StartWait\n");
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  if (LC_GeldKarte_ExtendCard(card)) {
    fprintf(stderr, "Could not extend card as STARCOS card\n");
    return 2;
  }

  fprintf(stderr, "INFO: Opening card\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Stopping wait\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Closing card\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);
  return 0;
}


int test31(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;

  GWEN_Logger_SetLevel(LC_LOGDOMAIN, GWEN_LoggerLevelDebug);
  GWEN_Logger_SetLevel(GWEN_LOGDOMAIN, GWEN_LoggerLevelInfo);
  GWEN_Logger_SetLevel(0, GWEN_LoggerLevelDebug);

  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  _check31(cl);
  fprintf(stderr, "Please abort the server now...\n");
  getchar();
  fprintf(stderr, "Retrying...\n");
  _check31(cl);

  GWEN_DB_Group_free(db); db=0;


  LC_Client_free(cl);
  return 0;
}



int test32(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  LC_CARD *card;
  LC_CLIENT_RESULT res;

  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending StartWait\n");
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  if (LC_DDVCard_ExtendCard(card)) {
    fprintf(stderr, "Could not extend card as DDV card\n");
    return 2;
  }

  fprintf(stderr, "INFO: Opening card\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Stopping wait\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Selecting EF...\n");
  res=LC_ProcessorCard_SelectEF(card, "EF_BNK");
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr,
          "Signkey: %d, %d\n",
          LC_DDVCard_GetSignKeyVersion(card),
          LC_DDVCard_GetSignKeyNumber(card));
  fprintf(stderr,
          "Cryptkey: %d, %d\n",
          LC_DDVCard_GetCryptKeyVersion(card),
          LC_DDVCard_GetCryptKeyNumber(card));

  fprintf(stderr, "INFO: Closing card\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Sleeping...\n");
  sleep(5);

  LC_Client_free(cl);
  return 0;
}



int testFS1(int argc, char **argv) {
  LC_FS *fs;
  GWEN_TYPE_UINT32 clid;
  int rv;
  GWEN_TYPE_UINT32 hid1;
  GWEN_TYPE_UINT32 hid2;

  GWEN_Logger_SetLevel(LC_LOGDOMAIN, GWEN_LoggerLevelDebug);
  GWEN_Logger_SetLevel(0, GWEN_LoggerLevelDebug);

  fprintf(stderr, "Creating file system\n");
  fs=LC_FS_new();
  fprintf(stderr, "Creating client\n");
  clid=LC_FS_CreateClient(fs);
  fprintf(stderr, "Client is %08x\n", clid);

  fprintf(stderr, "Creating folder1\n");
  rv=LC_FS_MkDir(fs, clid, "/testDir1",
		 LC_FS_MODE_RIGHTS_OWNER_EXEC |
		 LC_FS_MODE_RIGHTS_OWNER_WRITE |
		 LC_FS_MODE_RIGHTS_OWNER_READ,
		 &hid1);
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "Folder created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }

  fprintf(stderr, "Creating folder2\n");
  rv=LC_FS_MkDir(fs, clid, "/testDir1/testDir2",
		 LC_FS_MODE_RIGHTS_OWNER_EXEC |
		 LC_FS_MODE_RIGHTS_OWNER_WRITE |
		 LC_FS_MODE_RIGHTS_OWNER_READ,
		 &hid2);
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "Folder created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }

  fprintf(stderr, "Changing CurrentWorkingDir\n");
  rv=LC_FS_ChangeWorkingDir(fs, clid, "/testDir1/testDir2");
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "Folder selected.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }

  rv=LC_FS_Dump(fs, clid, "/", stderr, 2);
  if (rv!=LC_FS_ErrorNone) {
    fprintf(stderr, "Function not supported for file system (%d).\n", rv);
  }

  return 0;
}



int testFS2(int argc, char **argv) {
  LC_FS *fs;
  GWEN_TYPE_UINT32 clid;
  int rv;
  GWEN_TYPE_UINT32 hid1;
  GWEN_TYPE_UINT32 hid2;
  GWEN_TYPE_UINT32 hid3;
  LC_FS_MODULE *fsm;

  GWEN_Logger_SetLevel(LC_LOGDOMAIN, GWEN_LoggerLevelDebug);
  GWEN_Logger_SetLevel(0, GWEN_LoggerLevelDebug);

  fprintf(stderr, "Creating file system\n");
  fs=LC_FS_new();
  fprintf(stderr, "Creating client\n");
  clid=LC_FS_CreateClient(fs);
  fprintf(stderr, "Client is %08x\n", clid);


  fprintf(stderr, "Creating folder1\n");
  rv=LC_FS_MkDir(fs, clid, "/testDir1",
		 LC_FS_MODE_RIGHTS_OWNER_EXEC |
		 LC_FS_MODE_RIGHTS_OWNER_WRITE |
		 LC_FS_MODE_RIGHTS_OWNER_READ,
		 &hid1);
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "Folder created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }

  fsm=LC_FSFileModule_new("testfolder");
  rv=LC_FS_Mount(fs, fsm, "/testDir1");
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "Folder created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }

  fprintf(stderr, "Creating file\n");
  rv=LC_FS_CreateFile(fs, clid, "/testDir1/testFile",
                      LC_FS_MODE_RIGHTS_OWNER_WRITE |
                      LC_FS_MODE_RIGHTS_OWNER_READ,
                      &hid2);
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "File created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }

  fprintf(stderr, "Creating folder2\n");
  rv=LC_FS_MkDir(fs, clid, "/testDir1/testDir2",
                 LC_FS_MODE_RIGHTS_OWNER_EXEC |
                 LC_FS_MODE_RIGHTS_OWNER_WRITE |
                 LC_FS_MODE_RIGHTS_OWNER_READ,
                 &hid3);
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "Folder created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }


  rv=LC_FS_Dump(fs, clid, "/", stderr, 2);
  if (rv!=LC_FS_ErrorNone) {
    fprintf(stderr, "Function not supported for file system (%d).\n", rv);
  }

  return 0;
}



int testFS3(int argc, char **argv) {
  LC_FS *fs;
  GWEN_TYPE_UINT32 clid;
  int rv;
  GWEN_TYPE_UINT32 hid1;
  GWEN_TYPE_UINT32 hid2;
  GWEN_TYPE_UINT32 hid3;
  LC_FS_MODULE *fsm;
  GWEN_BUFFER *dbuf;

  GWEN_Logger_SetLevel(LC_LOGDOMAIN, GWEN_LoggerLevelDebug);
  GWEN_Logger_SetLevel(0, GWEN_LoggerLevelDebug);

  fprintf(stderr, "Creating file system\n");
  fs=LC_FS_new();
  fprintf(stderr, "Creating client\n");
  clid=LC_FS_CreateClient(fs);
  fprintf(stderr, "Client is %08x\n", clid);


  fprintf(stderr, "Creating folder1\n");
  rv=LC_FS_MkDir(fs, clid, "/testDir1",
		 LC_FS_MODE_RIGHTS_OWNER_EXEC |
		 LC_FS_MODE_RIGHTS_OWNER_WRITE |
		 LC_FS_MODE_RIGHTS_OWNER_READ,
		 &hid1);
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "Folder created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }

  fsm=LC_FSFileModule_new("testfolder");
  //LC_FSModule_SetMountFlags(fsm, LC_FS_MOUNT_FLAGS_READONLY);
  rv=LC_FS_Mount(fs, fsm, "/testDir1");
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "Folder created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }

  fprintf(stderr, "Opening file\n");
  rv=LC_FS_OpenFile(fs, clid, "/testDir1/README",
                    LC_FS_MODE_RIGHTS_OWNER_WRITE |
                    LC_FS_MODE_RIGHTS_OWNER_READ |
                    LC_FS_HANDLE_MODE_READ,
                    &hid2);
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "File opened.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }

  dbuf=GWEN_Buffer_new(0, 1024, 0, 1);
  rv=LC_FS_ReadFile(fs, clid, hid2, 0, 4410, dbuf);
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "File read.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }

  GWEN_Buffer_Dump(dbuf, stderr, 2);

  fprintf(stderr, "Creating folder2\n");
  rv=LC_FS_MkDir(fs, clid, "/testDir1/testDir2",
                 LC_FS_MODE_RIGHTS_OWNER_EXEC |
                 LC_FS_MODE_RIGHTS_OWNER_WRITE |
                 LC_FS_MODE_RIGHTS_OWNER_READ,
                 &hid3);
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "Folder created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }


  rv=LC_FS_Dump(fs, clid, "/", stderr, 2);
  if (rv!=LC_FS_ErrorNone) {
    fprintf(stderr, "Function not supported for file system (%d).\n", rv);
  }

  return 0;
}



int testFS4(int argc, char **argv) {
  LC_FS *fs;
  GWEN_TYPE_UINT32 clid;
  int rv;
  GWEN_TYPE_UINT32 hid1;
  GWEN_TYPE_UINT32 hid2;
  GWEN_TYPE_UINT32 hid3;
  LC_FS_MODULE *fsm;

  GWEN_Logger_SetLevel(LC_LOGDOMAIN, GWEN_LoggerLevelDebug);
  GWEN_Logger_SetLevel(0, GWEN_LoggerLevelDebug);

  fprintf(stderr, "Creating file system\n");
  fs=LC_FS_new();
  fprintf(stderr, "Creating client\n");
  clid=LC_FS_CreateClient(fs);
  fprintf(stderr, "Client is %08x\n", clid);


  fprintf(stderr, "Creating folder1\n");
  rv=LC_FS_MkDir(fs, clid, "/testDir1",
		 LC_FS_MODE_RIGHTS_OWNER_EXEC |
		 LC_FS_MODE_RIGHTS_OWNER_WRITE |
		 LC_FS_MODE_RIGHTS_OWNER_READ,
		 &hid1);
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "Folder created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }

  fsm=LC_FSFileModule_new("testfolder");
  rv=LC_FS_Mount(fs, fsm, "/testDir1");
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "Folder created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }

  fprintf(stderr, "Creating file\n");
  rv=LC_FS_CreateFile(fs, clid, "/testDir1/testFile1",
                      LC_FS_MODE_RIGHTS_OWNER_WRITE |
                      LC_FS_MODE_RIGHTS_OWNER_READ,
                      &hid2);
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "File created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }

  fprintf(stderr, "Creating folder2\n");
  rv=LC_FS_MkDir(fs, clid, "/testDir1/testDir2",
                 LC_FS_MODE_RIGHTS_OWNER_EXEC |
                 LC_FS_MODE_RIGHTS_OWNER_WRITE |
                 LC_FS_MODE_RIGHTS_OWNER_READ,
                 &hid3);
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "Folder created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }

  fprintf(stderr, "Creating file2\n");
  rv=LC_FS_CreateFile(fs, clid, "/testDir1/testDir2/testFile2",
                      LC_FS_MODE_RIGHTS_OWNER_WRITE |
                      LC_FS_MODE_RIGHTS_OWNER_READ,
                      &hid2);
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "File created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }


  rv=LC_FS_Dump(fs, clid, "/", stderr, 2);
  if (rv!=LC_FS_ErrorNone) {
    fprintf(stderr, "Function not supported for file system (%d).\n", rv);
  }

  return 0;
}



void _dumpStat(LC_FS_STAT *st, const char *s, int indent) {
  GWEN_TYPE_UINT32 fl;
  int i;
  
  for (i=0; i<indent; i++)
    fprintf(stdout, " ");
  
  fl=LC_FSStat_GetFileMode(st);
  if ((fl & LC_FS_MODE_FTYPE_MASK)==LC_FS_MODE_FTYPE_FILE)
    fprintf(stdout, "-");
  else if ((fl & LC_FS_MODE_FTYPE_MASK)==LC_FS_MODE_FTYPE_DIR)
    fprintf(stdout, "d");
  else {
    DBG_ERROR(0, "Unknown file type %08x (%08x)\n",
              fl, fl & LC_FS_MODE_FTYPE_MASK);
    fprintf(stdout, "?");
  }
  if (fl & LC_FS_MODE_RIGHTS_OWNER_READ)
    fprintf(stdout, "r");
  else
    fprintf(stdout, "-");
  if (fl & LC_FS_MODE_RIGHTS_OWNER_WRITE)
    fprintf(stdout, "w");
  else
    fprintf(stdout, "-");
  if (fl & LC_FS_MODE_RIGHTS_OWNER_EXEC)
    fprintf(stdout, "x");
  else
    fprintf(stdout, "-");
  if (fl & LC_FS_MODE_RIGHTS_GROUP_READ)
    fprintf(stdout, "r");
  else
    fprintf(stdout, "-");
  if (fl & LC_FS_MODE_RIGHTS_GROUP_WRITE)
    fprintf(stdout, "w");
  else
    fprintf(stdout, "-");
  if (fl & LC_FS_MODE_RIGHTS_GROUP_EXEC)
    fprintf(stdout, "x");
  else
    fprintf(stdout, "-");
  if (fl & LC_FS_MODE_RIGHTS_OTHER_READ)
    fprintf(stdout, "r");
  else
    fprintf(stdout, "-");
  if (fl & LC_FS_MODE_RIGHTS_OTHER_WRITE)
    fprintf(stdout, "w");
  else
    fprintf(stdout, "-");
  if (fl & LC_FS_MODE_RIGHTS_OTHER_EXEC)
    fprintf(stdout, "x");
  else
    fprintf(stdout, "-");
  
  fprintf(stdout, " ");
  
  fl=LC_FSStat_GetFileSize(st);
  fprintf(stdout, GWEN_TYPE_TMPL_UINT32 " ", fl);
  
  fprintf(stdout, "%s ", s);
  fprintf(stdout, "\n");
}



int _showFiles(LC_FS *fs, GWEN_TYPE_UINT32 clid,
               const char *dname, int indent) {
  int rv;
  GWEN_TYPE_UINT32 hid;

  DBG_ERROR(0, "Opening dir \"%s\"", dname);

  rv=LC_FS_OpenDir(fs, clid, dname, &hid);
  if (rv) {
    DBG_ERROR(0, "Error opening dir \"%s\": %d", dname, rv);
  }
  else {
    GWEN_STRINGLIST2 *sl;

    sl=GWEN_StringList2_new();
    rv=LC_FS_ReadDir(fs, clid, hid, sl);
    if (rv) {
      DBG_ERROR(0, "Error reading dir \"%s\": %d", dname, rv);
    }
    else {
      GWEN_STRINGLIST2_ITERATOR *sit;

      sit=GWEN_StringList2_First(sl);
      if (sit) {
        const char *s;

        s=GWEN_StringList2Iterator_Data(sit);
        while(s) {
          GWEN_BUFFER *nbuf;
          LC_FS_STAT *st=0;

          nbuf=GWEN_Buffer_new(0, 256, 0, 1);
          GWEN_Buffer_AppendString(nbuf, dname);
          if (strcasecmp(dname, "/")!=0)
            GWEN_Buffer_AppendString(nbuf, "/");
          GWEN_Buffer_AppendString(nbuf, s);
          rv=LC_FS_Stat(fs, clid, GWEN_Buffer_GetStart(nbuf), &st);
          if (rv) {
            DBG_ERROR(0, "Error stating entry \"%s\": %d",
                      GWEN_Buffer_GetStart(nbuf), rv);
            GWEN_Buffer_free(nbuf);
            break;
          }
          else {
            _dumpStat(st, s, indent);
          }
          GWEN_Buffer_free(nbuf);
          s=GWEN_StringList2Iterator_Next(sit);
        }
        GWEN_StringList2Iterator_free(sit);
      }
      else {
        DBG_ERROR(0, "Empty folder \"%s\"", dname);
      }

      sit=GWEN_StringList2_First(sl);
      if (sit) {
        const char *s;

        s=GWEN_StringList2Iterator_Data(sit);
        while(s) {
          GWEN_BUFFER *nbuf;
          LC_FS_STAT *st=0;

          nbuf=GWEN_Buffer_new(0, 256, 0, 1);
          GWEN_Buffer_AppendString(nbuf, dname);
          if (strcasecmp(dname, "/")!=0)
            GWEN_Buffer_AppendString(nbuf, "/");
          GWEN_Buffer_AppendString(nbuf, s);
          DBG_ERROR(0, "Stating entry \"%s\"",
                    GWEN_Buffer_GetStart(nbuf));
          rv=LC_FS_Stat(fs, clid, GWEN_Buffer_GetStart(nbuf), &st);
          if (rv) {
            DBG_ERROR(0, "Error stating entry \"%s\": %d",
                      GWEN_Buffer_GetStart(nbuf), rv);
            GWEN_Buffer_free(nbuf);
            break;
          }
          else {
            if (((LC_FSStat_GetFileMode(st) & LC_FS_MODE_FTYPE_MASK)
                ==LC_FS_MODE_FTYPE_DIR) &&
                (strcasecmp(s, ".")!=0) &&
                (strcasecmp(s, "..")!=0)) {
              fprintf(stderr, "Recursion\n");
              _showFiles(fs, clid, GWEN_Buffer_GetStart(nbuf),
                         indent+1);
            }
          }
          GWEN_Buffer_free(nbuf);
          s=GWEN_StringList2Iterator_Next(sit);
        }
        GWEN_StringList2Iterator_free(sit);
      }

    }
  }

  rv=LC_FS_CloseDir(fs, clid, hid);
  if (rv) {
    DBG_ERROR(0, "Error closing dir \"%s\": %d", dname, rv);
  }

  return 0;
}



int testFS5(int argc, char **argv) {
  LC_FS *fs;
  GWEN_TYPE_UINT32 clid;
  int rv;
  GWEN_TYPE_UINT32 hid1;
  GWEN_TYPE_UINT32 hid2;
  GWEN_TYPE_UINT32 hid3;
  LC_FS_MODULE *fsm;

  GWEN_Logger_SetLevel(LC_LOGDOMAIN, GWEN_LoggerLevelDebug);
  GWEN_Logger_SetLevel(0, GWEN_LoggerLevelDebug);

  fprintf(stderr, "Creating file system\n");
  fs=LC_FS_new();
  fprintf(stderr, "Creating client\n");
  clid=LC_FS_CreateClient(fs);
  fprintf(stderr, "Client is %08x\n", clid);


  fprintf(stderr, "Creating folder1\n");
  rv=LC_FS_MkDir(fs, clid, "/testDir1",
		 LC_FS_MODE_RIGHTS_OWNER_EXEC |
		 LC_FS_MODE_RIGHTS_OWNER_WRITE |
		 LC_FS_MODE_RIGHTS_OWNER_READ,
		 &hid1);
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "Folder created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }

  fsm=LC_FSFileModule_new("testfolder");
  rv=LC_FS_Mount(fs, fsm, "/testDir1");
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "Folder created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }

  fprintf(stderr, "Creating file\n");
  rv=LC_FS_CreateFile(fs, clid, "/testDir1/testFile1",
                      LC_FS_MODE_RIGHTS_OWNER_WRITE |
                      LC_FS_MODE_RIGHTS_OWNER_READ,
                      &hid2);
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "File created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }

  fprintf(stderr, "Creating folder2\n");
  rv=LC_FS_MkDir(fs, clid, "/testDir1/testDir2",
                 LC_FS_MODE_RIGHTS_OWNER_EXEC |
                 LC_FS_MODE_RIGHTS_OWNER_WRITE |
                 LC_FS_MODE_RIGHTS_OWNER_READ,
                 &hid3);
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "Folder created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }

  fprintf(stderr, "Creating file2\n");
  rv=LC_FS_CreateFile(fs, clid, "/testDir1/testDir2/testFile2",
                      LC_FS_MODE_RIGHTS_OWNER_WRITE |
                      LC_FS_MODE_RIGHTS_OWNER_READ,
                      &hid2);
  if (rv==LC_FS_ErrorNone) {
    fprintf(stderr, "File created.\n");
  }
  else {
    fprintf(stderr, "Error: %d\n", rv);
  }


  rv=LC_FS_Dump(fs, clid, "/", stderr, 2);
  if (rv!=LC_FS_ErrorNone) {
    fprintf(stderr, "Function not supported for file system (%d).\n", rv);
  }

  _showFiles(fs, clid, "/", 0);

  return 0;
}



int test33(int argc, char **argv) {
  LC_CLIENT *cl;
  LC_CARD *card;
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbData;
  int maxErrors;
  int currentErrors;
  int i;

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfigFile(cl, 0)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  fprintf(stderr, "INFO: Sending StartWait\n");
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  if (LC_Starcos_ExtendCard(card)) {
    fprintf(stderr, "Could not extend card as STARCOS card\n");
    return 2;
  }

  fprintf(stderr, "INFO: Opening card\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  dbData=LC_Starcos_GetCardDataAsDb(card);
  GWEN_DB_Dump(dbData, stderr, 2);

  fprintf(stderr, "Stopping wait\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Getting pin status...\n");
  for (i=0; i<256; i++) {
    fprintf(stderr, "Trying %3d (%02x)...\n", i, i);
    res=LC_Starcos_GetPinStatus(card, i, &maxErrors, &currentErrors);
    if (res==LC_Client_ResultOk) {
      fprintf(stderr, "Got it: %d (%02x)\n", i, i);
      break;
    }
  }

  fprintf(stderr, "Pin Status: MaxErrors=%d, CurrentErrors=%d\n",
          maxErrors, currentErrors);

  fprintf(stderr, "INFO: Closing card\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Sleeping...\n");
  sleep(5);

  LC_Client_free(cl);
  return 0;
}



int test34(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  LC_CARD *card;
  LC_CLIENT_RESULT res;

  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending StartWait\n");
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  if (LC_DDVCard_ExtendCard(card)) {
    fprintf(stderr, "Could not extend card as DDV card\n");
    return 2;
  }

  fprintf(stderr, "INFO: Opening card\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Stopping wait\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Verifying PIN...\n");
  res=LC_DDVCard_VerifyPin(card, "27246");
  //res=LC_DDVCard_SecureVerifyPin(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Closing card\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  LC_Client_free(cl);
  return 0;
}



int test35(int argc, char **argv) {
  LC_CLIENT *cl;
  GWEN_DB_NODE *db;
  LC_CARD *card;
  LC_CLIENT_RESULT res;

  db=GWEN_DB_Group_new("client");
  if (GWEN_DB_ReadFile(db,
                       "chipcardc.conf",
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not read file\n");
    return 1;
  }

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfig(cl, db)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  GWEN_DB_Group_free(db); db=0;

  fprintf(stderr, "INFO: Sending StartWait\n");
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "INFO: Waiting for card\n");
  card=LC_Client_WaitForNextCard(cl, 30000);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }

  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  if (LC_DDVCard_ExtendCard(card)) {
    fprintf(stderr, "Could not extend card as DDV card\n");
    return 2;
  }

  fprintf(stderr, "INFO: Opening card\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Stopping wait\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  // do something here

  fprintf(stderr, "INFO: Closing card\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  LC_Client_free(cl);
  return 0;
}



int stressTest1(int argc, char **argv) {
  LC_CLIENT *cl;
  int loops;

  cl=LC_Client_new("lctest", "0.1", 0);
  if (LC_Client_ReadConfigFile(cl, 0)) {
    fprintf(stderr, "Error reading configuration.\n");
    LC_Client_free(cl);
    return 1;
  }

  for (loops=1; loops<=100; loops++) {
    LC_CARD *card;
    LC_CLIENT_RESULT res;
    int maxErrors;
    int currentErrors;

    fprintf(stderr, "INFO: Loop %d\n", loops);
    res=LC_Client_StartWait(cl, 0, 0);
    if (res!=LC_Client_ResultOk) {
      fprintf(stderr, "ERROR: Wait timed out.\n");
      LC_Client_free(cl);
      return 2;
    }

    card=LC_Client_WaitForNextCard(cl, 30000);
    if (!card) {
      fprintf(stderr, "ERROR: No card found.\n");
      LC_Client_free(cl);
      return 2;
    }
  
    if (LC_Starcos_ExtendCard(card)) {
      fprintf(stderr, "Could not extend card as STARCOS card\n");
      LC_Card_free(card);
      LC_Client_free(cl);
      return 2;
    }

    res=LC_Card_Open(card);
    if (res!=LC_Client_ResultOk) {
      fprintf(stderr, "ERROR: Wait timed out.\n");
      LC_Card_free(card);
      LC_Client_free(cl);
      return 2;
    }

    res=LC_Client_StopWait(cl);
    if (res!=LC_Client_ResultOk) {
      fprintf(stderr, "ERROR: Wait timed out.\n");
      LC_Card_free(card);
      LC_Client_free(cl);
      return 2;
    }
  
    res=LC_Starcos_GetPinStatus(card, 0x90, &maxErrors, &currentErrors);
    if (res!=LC_Client_ResultOk) {
      fprintf(stderr, "ERROR: Could not get pin status.\n");
      LC_Card_free(card);
      LC_Client_free(cl);
      return 2;
    }

    res=LC_Card_Close(card);
    if (res!=LC_Client_ResultOk) {
      fprintf(stderr, "ERROR: Wait timed out.\n");
      LC_Card_free(card);
      LC_Client_free(cl);
      return 2;
    }

    LC_Card_free(card);
  }

  LC_Client_free(cl);
  return 0;
}



int main(int argc, char **argv) {

  GWEN_Logger_SetLevel(LC_LOGDOMAIN, GWEN_LoggerLevelNotice);
  GWEN_Logger_SetLevel(0, GWEN_LoggerLevelNotice);

  if (argc<2) {
    fprintf(stderr, "At least command name needed.\n");
    return 1;
  }

  if (strcasecmp(argv[1], "test1")==0)
    return test1(argc, argv);
  else if (strcasecmp(argv[1], "test2")==0)
    return test2(argc, argv);
  else if (strcasecmp(argv[1], "test3")==0)
    return test3(argc, argv);
  else if (strcasecmp(argv[1], "test4")==0)
    return test4(argc, argv);
  else if (strcasecmp(argv[1], "test5")==0)
    return test5(argc, argv);
  else if (strcasecmp(argv[1], "test6")==0)
    return test6(argc, argv);
  else if (strcasecmp(argv[1], "test7")==0)
    return test7(argc, argv);
  else if (strcasecmp(argv[1], "test8")==0)
    return test8(argc, argv);
  else if (strcasecmp(argv[1], "test10")==0)
    return test10(argc, argv);
  else if (strcasecmp(argv[1], "test11")==0)
    return test11(argc, argv);
  else if (strcasecmp(argv[1], "test12")==0)
    return test12(argc, argv);
  else if (strcasecmp(argv[1], "test13")==0)
    return test13(argc, argv);
  else if (strcasecmp(argv[1], "test14")==0)
    return test14(argc, argv);
  else if (strcasecmp(argv[1], "test15")==0)
    return test15(argc, argv);
  else if (strcasecmp(argv[1], "test16")==0)
    return test16(argc, argv);
  else if (strcasecmp(argv[1], "test17")==0)
    return test17(argc, argv);
  else if (strcasecmp(argv[1], "test18")==0)
    return test18(argc, argv);
  else if (strcasecmp(argv[1], "test19")==0)
    return test19(argc, argv);
  else if (strcasecmp(argv[1], "test20")==0)
    return test20(argc, argv);
  else if (strcasecmp(argv[1], "test21")==0)
    return test21(argc, argv);
  else if (strcasecmp(argv[1], "test22")==0)
    return test22(argc, argv);
  else if (strcasecmp(argv[1], "test23")==0)
    return test23(argc, argv);
  else if (strcasecmp(argv[1], "test24")==0)
    return test24(argc, argv);
  else if (strcasecmp(argv[1], "test25")==0)
    return test25(argc, argv);
  else if (strcasecmp(argv[1], "test26")==0)
    return test26(argc, argv);
  else if (strcasecmp(argv[1], "test27")==0)
    return test27(argc, argv);
  else if (strcasecmp(argv[1], "test28")==0)
    return test28(argc, argv);
  else if (strcasecmp(argv[1], "test29")==0)
    return test29(argc, argv);
  else if (strcasecmp(argv[1], "test30")==0)
    return test30(argc, argv);
  else if (strcasecmp(argv[1], "test31")==0)
    return test31(argc, argv);
  else if (strcasecmp(argv[1], "test32")==0)
    return test32(argc, argv);
  else if (strcasecmp(argv[1], "fs")==0)
    return testFS1(argc, argv);
  else if (strcasecmp(argv[1], "fs2")==0)
    return testFS2(argc, argv);
  else if (strcasecmp(argv[1], "fs3")==0)
    return testFS3(argc, argv);
  else if (strcasecmp(argv[1], "fs4")==0)
    return testFS4(argc, argv);
  else if (strcasecmp(argv[1], "fs5")==0)
    return testFS5(argc, argv);
  else if (strcasecmp(argv[1], "test33")==0)
    return test33(argc, argv);
  else if (strcasecmp(argv[1], "test34")==0)
    return test34(argc, argv);
  else if (strcasecmp(argv[1], "test35")==0)
    return test35(argc, argv);
  else if (strcasecmp(argv[1], "stress1")==0)
    return stressTest1(argc, argv);
  else {
    fprintf(stderr, "Unknown command \"%s\"\n", argv[1]);
    return 1;
  }

  return 0;
}


