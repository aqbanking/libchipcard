
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "io/lcc/clientlcc.h"
#include "io/pcsc/clientpcsc.h"

#include <chipcard/client/cards/ddvcard.h>
#include <chipcard/client/cards/egkcard.h>

#include <gwenhywfar/text.h>
#include <gwenhywfar/buffer.h>

#include <stdio.h>




#define LC_CLIENT_WITH_LCC
#ifdef HAVE_PCSC
# define LC_CLIENT_WITH_PCSC
#endif


#ifdef LC_CLIENT_WITH_LCC

int testLcc1(int argc, char **argv) {
  LC_CLIENT *cl;

  cl=LC_ClientLcc_new("test", "0.1");
  if (!cl) {
    fprintf(stderr, "Could not create client.\n");
    return 1;
  }
  return 0;
}



int testLcc2(int argc, char **argv) {
  LC_CLIENT *cl;
  LC_CLIENT_RESULT res;

  cl=LC_ClientLcc_new("test", "0.1");
  if (!cl) {
    fprintf(stderr, "ERROR: Could not create client.\n");
    return 1;
  }

  res=LC_Client_Init(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not init client (%d).\n",
            res);
    return 2;
  }

  return 0;
}



int testLcc3(int argc, char **argv) {
  LC_CLIENT *cl;
  LC_CLIENT_RESULT res;

  cl=LC_ClientLcc_new("test", "0.1");
  if (!cl) {
    fprintf(stderr, "ERROR: Could not create client.\n");
    return 1;
  }

  res=LC_Client_Init(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not init client (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_Fini(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not fini client (%d).\n",
            res);
    return 2;
  }

  return 0;
}



int testLcc4(int argc, char **argv) {
  LC_CLIENT *cl;
  LC_CLIENT_RESULT res;

  cl=LC_ClientLcc_new("test", "0.1");
  if (!cl) {
    fprintf(stderr, "ERROR: Could not create client.\n");
    return 1;
  }

  res=LC_Client_Init(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not init client (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_Start(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not start using cards (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_Stop(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not stop using cards (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_Fini(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not fini client (%d).\n",
            res);
    return 2;
  }

  return 0;
}



int testLcc5(int argc, char **argv) {
  LC_CLIENT *cl;
  LC_CLIENT_RESULT res;
  LC_CARD *card;

  cl=LC_ClientLcc_new("test", "0.1");
  if (!cl) {
    fprintf(stderr, "ERROR: Could not create client.\n");
    return 1;
  }

  res=LC_Client_Init(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not init client (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_Start(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not start using cards (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_GetNextCard(cl, &card, 20);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: No card found (%d).\n",
            res);
    return 2;
  }

  fprintf(stderr, "Got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  res=LC_Client_ReleaseCard(cl, card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Unable to release card (%d).\n",
            res);
    return 2;
  }
  LC_Card_free(card);

  res=LC_Client_Stop(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not stop using cards (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_Fini(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not fini client (%d).\n",
            res);
    return 2;
  }

  return 0;
}



int testLcc6(int argc, char **argv) {
  LC_CLIENT *cl;
  LC_CLIENT_RESULT res;
  LC_CARD *card;
  GWEN_BUFFER *mbuf;
  GWEN_DB_NODE *dbRecord;

  cl=LC_ClientLcc_new("test", "0.1");
  if (!cl) {
    fprintf(stderr, "ERROR: Could not create client.\n");
    return 1;
  }

  res=LC_Client_Init(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not init client (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_Start(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not start using cards (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_GetNextCard(cl, &card, 20);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: No card found (%d).\n",
            res);
    return 2;
  }

  fprintf(stderr, "Got this card:\n");
  LC_Card_Dump(card, stderr, 2);


  if (LC_DDVCard_ExtendCard(card)) {
    fprintf(stderr, "ERROR: Could not extend card as DDV card\n");
    return 2;
  }

  fprintf(stderr, "INFO: Opening card\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Unable to open card (%d).\n", res);
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Selecting EF...\n");
  res=LC_Card_SelectEf(card, "EF_BNK");
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


  res=LC_Client_ReleaseCard(cl, card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Unable to release card (%d).\n",
            res);
    return 2;
  }
  LC_Card_free(card);


  res=LC_Client_Stop(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not stop using cards (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_Fini(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not fini client (%d).\n",
            res);
    return 2;
  }

  return 0;
}



int testLcc7(int argc, char **argv) {
  LC_CLIENT *cl;
  LC_CLIENT_RESULT res;
  LC_CARD *card;
  uint8_t cmdSelectDF[]={
      0x00, 0xa4, 0x04, 0x00,
      0x07,
      0xa0, 0x00, 0x00, 0x00, 0x04, 0x30, 0x60, 0x00};
  uint8_t cmdReadRecord[]={ 0x00, 0xb2, 0x00, 0x00, 0x00};
  GWEN_BUFFER *mbuf;
  int i;
  int j;

  cl=LC_ClientLcc_new("test", "0.1");
  if (!cl) {
    fprintf(stderr, "ERROR: Could not create client.\n");
    return 1;
  }

  res=LC_Client_Init(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not init client (%d).\n",
            res);
    return 2;
  }

  fprintf(stderr, "Client Start\n");
  res=LC_Client_Start(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not start using cards (%d).\n",
            res);
    return 2;
  }

  fprintf(stderr, "GetNextCard\n");
  res=LC_Client_GetNextCard(cl, &card, 20);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: No card found (%d).\n",
            res);
    return 2;
  }

  fprintf(stderr, "Got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  fprintf(stderr, "INFO: Opening card\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Unable to open card (%d).\n", res);
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  mbuf=GWEN_Buffer_new(0, 256, 0, 1);

  res=LC_Card_ExecApdu(card, (const char*)cmdSelectDF, sizeof(cmdSelectDF),
		       mbuf, LC_Client_CmdTargetCard, 10000);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Wait timed out.\n");
    return 2;
  }

  for (i=1; i<32; i++) {
    cmdReadRecord[3]=(i<<3) | 4; /* SFI */
    for (j=1; j<16; j++) {
      cmdReadRecord[2]=j; /* record number */
      res=LC_Card_ExecApdu(card, (const char*)cmdReadRecord, sizeof(cmdReadRecord),
			   mbuf, LC_Client_CmdTargetCard, 10000);
      if (res!=LC_Client_ResultOk) {
	fprintf(stderr, "ERROR: Wait timed out.\n");
      }
      else {
	fprintf(stdout, "SFI=%d, Record=%d:\n", i, j);
	GWEN_Text_DumpString(GWEN_Buffer_GetStart(mbuf), GWEN_Buffer_GetUsedBytes(mbuf),
			     stdout, 2);
      }
      GWEN_Buffer_Reset(mbuf);
    }
  }

  res=LC_Client_ReleaseCard(cl, card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Unable to release card (%d).\n",
            res);
    return 2;
  }
  LC_Card_free(card);


  res=LC_Client_Stop(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not stop using cards (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_Fini(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not fini client (%d).\n",
            res);
    return 2;
  }

  return 0;
}


#endif



#ifdef LC_CLIENT_WITH_PCSC
int testPcsc1(int argc, char **argv) {
  LC_CLIENT *cl;

  cl=LC_ClientPcsc_new("test", "0.1");
  if (!cl) {
    fprintf(stderr, "Could not create client.\n");
    return 1;
  }
  return 0;
}



int testPcsc2(int argc, char **argv) {
  LC_CLIENT *cl;
  LC_CLIENT_RESULT res;

  cl=LC_ClientPcsc_new("test", "0.1");
  if (!cl) {
    fprintf(stderr, "ERROR: Could not create client.\n");
    return 1;
  }

  res=LC_Client_Init(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not init client (%d).\n",
            res);
    return 2;
  }

  return 0;
}



int testPcsc3(int argc, char **argv) {
  LC_CLIENT *cl;
  LC_CLIENT_RESULT res;

  cl=LC_ClientPcsc_new("test", "0.1");
  if (!cl) {
    fprintf(stderr, "ERROR: Could not create client.\n");
    return 1;
  }

  res=LC_Client_Init(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not init client (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_Fini(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not fini client (%d).\n",
            res);
    return 2;
  }

  return 0;
}



int testPcsc4(int argc, char **argv) {
  LC_CLIENT *cl;
  LC_CLIENT_RESULT res;

  cl=LC_ClientPcsc_new("test", "0.1");
  if (!cl) {
    fprintf(stderr, "ERROR: Could not create client.\n");
    return 1;
  }

  res=LC_Client_Init(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not init client (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_Start(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not start using cards (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_Stop(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not stop using cards (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_Fini(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not fini client (%d).\n",
            res);
    return 2;
  }

  return 0;
}



int testPcsc5(int argc, char **argv) {
  LC_CLIENT *cl;
  LC_CLIENT_RESULT res;
  LC_CARD *card;

  cl=LC_ClientPcsc_new("test", "0.1");
  if (!cl) {
    fprintf(stderr, "ERROR: Could not create client.\n");
    return 1;
  }

  res=LC_Client_Init(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not init client (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_Start(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not start using cards (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_GetNextCard(cl, &card, 20);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: No card found (%d).\n",
            res);
    return 2;
  }

  fprintf(stderr, "Got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  res=LC_Client_ReleaseCard(cl, card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Unable to release card (%d).\n",
            res);
    return 2;
  }
  LC_Card_free(card);

  res=LC_Client_Stop(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not stop using cards (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_Fini(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not fini client (%d).\n",
            res);
    return 2;
  }

  return 0;
}



int testPcsc6(int argc, char **argv) {
  LC_CLIENT *cl;
  LC_CLIENT_RESULT res;
  LC_CARD *card;
  GWEN_BUFFER *mbuf;
  GWEN_DB_NODE *dbRecord;

  cl=LC_ClientPcsc_new("test", "0.1");
  if (!cl) {
    fprintf(stderr, "ERROR: Could not create client.\n");
    return 1;
  }

  res=LC_Client_Init(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not init client (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_Start(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not start using cards (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_GetNextCard(cl, &card, 20);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: No card found (%d).\n",
            res);
    return 2;
  }

  fprintf(stderr, "Got this card:\n");
  LC_Card_Dump(card, stderr, 2);


  if (LC_DDVCard_ExtendCard(card)) {
    fprintf(stderr, "ERROR: Could not extend card as DDV card\n");
    return 2;
  }

  fprintf(stderr, "INFO: Opening card\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Unable to open card (%d).\n", res);
    return 2;
  }
  fprintf(stderr, "Response was %d\n", res);

  fprintf(stderr, "Selecting EF...\n");
  res=LC_Card_SelectEf(card, "EF_BNK");
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


  res=LC_Client_ReleaseCard(cl, card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Unable to release card (%d).\n",
            res);
    return 2;
  }
  LC_Card_free(card);


  res=LC_Client_Stop(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not stop using cards (%d).\n",
            res);
    return 2;
  }

  res=LC_Client_Fini(cl);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr, "ERROR: Could not fini client (%d).\n",
            res);
    return 2;
  }

  return 0;
}
#endif



int main(int argc, char **argv) {
  int rv;
  int tcount=0;
  int tfailed=0;

  rv=0;

#ifdef LC_CLIENT_WITH_LCC
  return testLcc7(argc, argv);

  tcount++;
  rv=testLcc1(argc, argv);
  if (rv)
    tfailed++;

  tcount++;
  rv=testLcc2(argc, argv);
  if (rv)
    tfailed++;

  tcount++;
  rv=testLcc3(argc, argv);
  if (rv)
    tfailed++;

  tcount++;
  rv=testLcc4(argc, argv);
  if (rv)
    tfailed++;

  tcount++;
  rv=testLcc5(argc, argv);
  if (rv)
    tfailed++;
#endif

#ifdef LC_CLIENT_WITH_PCSC
  tcount++;
  rv=testPcsc1(argc, argv);
  if (rv)
    tfailed++;

  tcount++;
  rv=testPcsc2(argc, argv);
  if (rv)
    tfailed++;

  tcount++;
  rv=testPcsc3(argc, argv);
  if (rv)
    tfailed++;

  tcount++;
  rv=testPcsc4(argc, argv);
  if (rv)
    tfailed++;

  tcount++;
  rv=testPcsc5(argc, argv);
  if (rv)
    tfailed++;

  tcount++;
  rv=testPcsc6(argc, argv);
  if (rv)
    tfailed++;

#endif

  if (tfailed==tcount) {
    fprintf(stderr, "FAILED: All tests failed.\n");
    return 2;
  }

  if (tfailed) {
    fprintf(stderr, "FAILED: %d out of %d tests failed.\n",
            tfailed, tcount);
    return 2;
  }

  fprintf(stderr, "PASSED: All %d tests passed.\n", tcount);
  return 0;
}




