


static int storeCardData(const char *fname,
                         LC_CARD *cd,
			 int dosMode,
			 GWEN_DB_NODE *dbData) {
  FILE *f;
  GWEN_BUFFER *dbuf;
  GWEN_TIME *ti;
  const char *CRLF;
  const char *s;

  ti=GWEN_CurrentTime();
  assert(ti);

  if (dosMode)
    CRLF="\r\n";
  else
    CRLF="\n";

  if (fname) {
    f=fopen(fname, "w+");
    if (f==0) {
      fprintf(stderr,
	      "Could not create file \"%s\", reason: %s\n",
	      fname,
	      strerror(errno));
      GWEN_Time_free(ti);
      return -1;
    }
  }
  else
    f=stdout;

  /* header */
  fprintf(f,"Version:libchipcard4-"CHIPCARD_VERSION_FULL_STRING"%s", CRLF);
  dbuf=GWEN_Buffer_new(0, 32, 0, 1);
  GWEN_Time_toString(ti, "DD.MM.YYYY", dbuf);
  fprintf(f, "Datum:%s%s", GWEN_Buffer_GetStart(dbuf), CRLF);
  GWEN_Buffer_Reset(dbuf);
  GWEN_Time_toString(ti, "hh:mm:ss", dbuf);
  fprintf(f, "Zeit:%s%s", GWEN_Buffer_GetStart(dbuf), CRLF);
  GWEN_Time_free(ti);
  GWEN_Buffer_free(dbuf); dbuf=0;
  fprintf(f, "Lesertyp:%s%s", LC_Card_GetReaderType(cd), CRLF);

  /* insurance data */
  fprintf(f,"KK-Name:%s%s",
	  GWEN_DB_GetCharValue(dbData, "insuranceCompanyName", 0, ""),
	  CRLF);
  fprintf(f,"KK-Nummer:%s%s",
	  GWEN_DB_GetCharValue(dbData, "insuranceCompanyCode", 0, ""),
	  CRLF);
  /* this is not the card number but the "Vertragskassennummer" */
  fprintf(f,"VKNR:%s%s",
	  GWEN_DB_GetCharValue(dbData, "cardNumber", 0, ""),
	  CRLF);
  fprintf(f,"V-Nummer:%s%s",
	  GWEN_DB_GetCharValue(dbData, "insuranceNumber", 0, ""),
	  CRLF);
  fprintf(f,"V-Status:%s%s",
	  GWEN_DB_GetCharValue(dbData, "insuranceState", 0, ""),
	  CRLF);
  s=GWEN_DB_GetCharValue(dbData, "eastOrWest", 0, "");
  fprintf(f,"V-Statusergaenzung:%s%s", s, CRLF);
  if (s) {
    const char *x=0;

    switch(*s) {
    case '1': x="west"; break;
    case '9': x="ost"; break;
    case '6': x="BVG"; break;
    case '7': x="SVA, nach Aufwand, dt.-nl Grenzgaenger"; break;
    case '8': x="SVA, pauschal"; break;
    case 'M': x="DMP Diabetes mellitus Typ 2, west"; break;
    case 'X': x="DMP Diabetes mellitus Typ 2, ost"; break;
    case 'A': x="DMP Brustkrebs, west"; break;
    case 'C': x="DMP Brustkrebs, ost"; break;
    case 'K': x="DMP KHK, west"; break;
    case 'L': x="DMP KHK, ost"; break;
    case '4': x="nichtversicherter Sozialhilfe-Empfaenger"; break;
    case 'E': x="DMP Diabetes mellitus Typ 1, west"; break;
    case 'N': x="DMP Diabetes mellitus Typ 1, ost"; break;
    case 'D': x="DMP Asthma bronchiale, west"; break;
    case 'F': x="DMP Asthma bronchiale, ost"; break;
    case 'S': x="DMP COPD, west"; break;
    case 'P': x="DMP COPD, ost"; break;
    default:  x=0;
    }
    if (x)
      fprintf(f,"V-Status-Erlaeuterung:%s%s", x, CRLF);
  }
  fprintf(f,"Titel:%s%s",
          GWEN_DB_GetCharValue(dbData, "title", 0, ""), CRLF);
  fprintf(f,"Vorname:%s%s",
          GWEN_DB_GetCharValue(dbData, "foreName", 0, ""), CRLF);
  fprintf(f,"Namenszusatz:%s%s",
          GWEN_DB_GetCharValue(dbData, "nameSuffix", 0, ""), CRLF);
  fprintf(f,"Familienname:%s%s",
	  GWEN_DB_GetCharValue(dbData, "name", 0, ""), CRLF);
  fprintf(f,"Geburtsdatum:%s%s",
          GWEN_DB_GetCharValue(dbData, "dateOfBirth", 0, ""), CRLF);
  fprintf(f,"Strasse:%s%s",
          GWEN_DB_GetCharValue(dbData, "addrStreet", 0, ""), CRLF);
  fprintf(f,"Laendercode:%s%s",
          GWEN_DB_GetCharValue(dbData, "addrState", 0, ""), CRLF);
  fprintf(f,"PLZ:%s%s",
          GWEN_DB_GetCharValue(dbData, "addrPostalCode", 0, ""), CRLF);
  fprintf(f,"Ort:%s%s",
          GWEN_DB_GetCharValue(dbData, "addrCity", 0, ""), CRLF);
  fprintf(f,"gueltig-bis:%s%s",
          GWEN_DB_GetCharValue(dbData, "bestBefore", 0, ""), CRLF);
  fprintf(f,"Pruefsumme-gueltig:ja%s", CRLF);
  fprintf(f,"Kommentar:derzeit keiner%s", CRLF);

  if (fname) {
    if (fclose(f)) {
      DBG_ERROR(0, "Could not close file \"%s\", reason: \n %s",
		fname,
		strerror(errno));
      return -1;
    }
  }

  return 0;
}



int kvkRead(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs){
  LC_CARD *card=0;
  LC_CLIENT_RESULT res;
  int rv;
  int v;
  const char *fname;
  GWEN_DB_NODE *dbData;
  int i;
  const char *s;
  uint32_t cardId;
  int dobeep;
  int dosMode;

  v=GWEN_DB_GetIntValue(dbArgs, "verbosity", 0, 0);
  cardId=GWEN_DB_GetIntValue(dbArgs, "cardId", 0, 0);
  dobeep=GWEN_DB_GetIntValue(dbArgs, "beep", 0, 0);
  dosMode=GWEN_DB_GetIntValue(dbArgs, "dosMode", 0, 0);

  if (v>1)
    fprintf(stderr, "Connecting to server.\n");
  res=LC_Client_Start(cl);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "StartWait");
    return RETURNVALUE_WORK;
  }
  if (v>1)
    fprintf(stderr, "Connected.\n");

  if (cardId==0) {
    for (i=0;;i++) {
      if (v>0)
	fprintf(stderr, "Waiting for card...\n");
      res=LC_Client_GetNextCard(cl, &card, 20);
      if (res!=LC_Client_ResultOk) {
	showError(card, res, "GetNextCard");
	errorBeep();
	return RETURNVALUE_WORK;
      }
      if (v>0)
	fprintf(stderr, "Found a card.\n");
  
      s=LC_Card_GetCardType(card);
      assert(s);
      if (strcasecmp(s, "memory")==0)
	break;
  
      if (v>0)
	fprintf(stderr, "Not a memory card, releasing.\n");
      res=LC_Client_ReleaseCard(cl, card);
      if (res!=LC_Client_ResultOk) {
	showError(card, res, "ReleaseCard");
	errorBeep();
	return RETURNVALUE_WORK;
      }
      LC_Card_free(card);
  
      if (i>15) {
	fprintf(stderr, "ERROR: No card found.\n");
	errorBeep();
	return RETURNVALUE_WORK;
      }
    } /* for */
  }
  else {
    for (i=0;;i++) {
      if (v>0)
	fprintf(stderr, "Waiting for card...\n");
      res=LC_Client_GetNextCard(cl, &card, 2);
      if (res!=LC_Client_ResultOk) {
	showError(card, res, "GetNextCard");
	errorBeep();
	return RETURNVALUE_WORK;
      }
      if (v>0)
	fprintf(stderr, "Found a card.\n");

      if (LC_Card_GetCardId(card)!=cardId) {
	if (v>0)
	  fprintf(stderr, "Not the wanted card (%08x), releasing.\n",
		  cardId);
      }
      else {
	s=LC_Card_GetCardType(card);
	assert(s);
	if (strcasecmp(s, "memory")==0)
	  break;
	fprintf(stderr,
		"The requested card is not a "
		"German medical card, aborting.\n");
	LC_Client_ReleaseCard(cl, card);
	LC_Card_free(card);
	errorBeep();
	return RETURNVALUE_WORK;
      }
      res=LC_Client_ReleaseCard(cl, card);
      if (res!=LC_Client_ResultOk) {
	showError(card, res, "ReleaseCard");
	errorBeep();
	return RETURNVALUE_WORK;
      }
      LC_Card_free(card);
  
      if (i>15) {
	fprintf(stderr, "ERROR: No card found.\n");
	errorBeep();
	return RETURNVALUE_WORK;
      }
    } /* for */
  }

  /* extend card */
  rv=LC_KVKCard_ExtendCard(card);
  if (rv) {
    fprintf(stderr, "Could not extend card as German medical card\n");
    errorBeep();
    return RETURNVALUE_WORK;
  }

  /* stop waiting */
  if (v>1)
    fprintf(stderr, "Telling the server that we need no more cards.\n");
  res=LC_Client_Stop(cl);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "Stop");
    errorBeep();
    return RETURNVALUE_WORK;
  }

  /* open card */
  if (v>0)
    fprintf(stderr, "Opening card.\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr,
            "ERROR: Error executing command CardOpen (%d).\n",
            res);
    errorBeep();
    return RETURNVALUE_WORK;
  }
  if (v>0)
    fprintf(stderr, "Card is a German medical card as expected.\n");

  dbData=LC_KVKCard_GetCardData(card);
  if (!dbData) {
    fprintf(stderr, "ERROR: No card data available.\n");
    LC_Card_Close(card);
    errorBeep();
    return RETURNVALUE_WORK;
  }

  /* open file */
  if (v>0)
    fprintf(stderr, "Writing data to file\n");
  fname=GWEN_DB_GetCharValue(dbArgs, "fileName", 0, 0);
  if (storeCardData(fname, card, dosMode, dbData)) {
    fprintf(stderr, "ERROR: Could not write to file.\n");
    LC_Card_Close(card);
    LC_Client_ReleaseCard(cl, card);
    LC_Card_free(card);
    errorBeep();
    return RETURNVALUE_WORK;
  }

  if (v>1)
    fprintf(stderr, "Data written.\n");

  okBeep();

  /* close card */
  if (v>0)
    fprintf(stderr, "Closing card.\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "CardClose");
    LC_Client_ReleaseCard(cl, card);
    LC_Card_free(card);
    return RETURNVALUE_WORK;
  }
  else
    if (v>1)
      fprintf(stderr, "Card closed.\n");

  if (v>0)
    fprintf(stderr, "Releasing card.\n");
  res=LC_Client_ReleaseCard(cl, card);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "ReleaseCard");
  }
  LC_Card_free(card);

  /* finished */
  if (v>1)
    fprintf(stderr, "Finished.\n");
  return 0;
}



