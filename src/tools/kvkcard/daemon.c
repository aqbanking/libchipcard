

#ifndef OS_WIN32


static void signalHandler(int s) {
  if (s==SIGCHLD) {
    int status=0;

    waitpid(-1, &status, WNOHANG);
  }
}




static void replaceVar(const char *path,
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
}



static int callForCard(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs, uint32_t cid){
  const char *prg;
  const char *args;
  int v;
  int dosMode;

  v=GWEN_DB_GetIntValue(dbArgs, "verbosity", 0, 0);
  dosMode=GWEN_DB_GetIntValue(dbArgs, "dosMode", 0, 0);
  prg=GWEN_DB_GetCharValue(dbArgs, "program", 0, 0);
  args=GWEN_DB_GetCharValue(dbArgs, "args", 0, 0);
  if (!prg || *prg==0) {
    prg="kvkcard";
    if (dosMode)
      args="read -b -d -c @cardid@ -f kvkcard-@cardid@.dat";
    else
      args="read -b -c @cardid@ -f kvkcard-@cardid@.dat";
  }

  if (prg && *prg) {
    GWEN_PROCESS *pr;
    GWEN_PROCESS_STATE st;
    GWEN_BUFFER *buf;
    char numbuf[20];

    snprintf(numbuf, sizeof(numbuf)-1, "0x%08x", cid);
    numbuf[sizeof(numbuf)-1]=0;
    buf=GWEN_Buffer_new(0, 256, 0, 1);
    replaceVar(args, "cardid", numbuf, buf);

    pr=GWEN_Process_new();
    if (v>2)
      fprintf(stderr, "Calling this for card %08x: [%s %s].\n",
	      cid, prg, GWEN_Buffer_GetStart(buf));
    st=GWEN_Process_Start(pr, prg, GWEN_Buffer_GetStart(buf));
    GWEN_Buffer_free(buf);
    if (st!=GWEN_ProcessStateRunning) {
      fprintf(stderr, "KVKD: Could not start process (%d)\n", st);
      GWEN_Process_free(pr);
      errorBeep();
      return RETURNVALUE_WORK;
    }

    /* we aren't interested in the result of the process */
    GWEN_Process_free(pr);
  }
  return 0;
}




int kvkDaemon(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs){
  int v;
  LC_CLIENT_RESULT res;
#ifdef HAVE_SIGACTION
  struct sigaction sa;

  sa.sa_handler=signalHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags=0;
  if (sigaction(SIGCHLD, &sa, 0)) {
    fprintf(stderr, "sigaction(): %s\n", strerror(errno));
    return RETURNVALUE_SETUP;
  }
#endif

  v=GWEN_DB_GetIntValue(dbArgs, "verbosity", 0, 0);
  if (v>1)
    fprintf(stderr, "Connecting to server.\n");
  res=LC_Client_Start(cl);
  if (res!=LC_Client_ResultOk) {
    showError(NULL, res, "StartWait");
    return RETURNVALUE_WORK;
  }
  if (v>1)
    fprintf(stderr, "Connected.\n");

  for (;;) {
    LC_CARD *card=0;

    if (v>0)
      fprintf(stderr, "Waiting for card...\n");
    res=LC_Client_GetNextCard(cl, &card, 20);
    if (res==LC_Client_ResultOk) {
      uint32_t cardId;

      if (v>0)
	fprintf(stderr, "Found a card...\n");
      cardId=LC_Card_GetCardId(card);
      res=LC_Client_ReleaseCard(cl, card);
      if (res!=LC_Client_ResultOk) {
	showError(card, res, "ReleaseCard");
      }
      LC_Card_free(card);

      if (cardId) {
	if (v>0)
	  fprintf(stderr, "Handling card %08x...\n", cardId);
	/* handle card */
	if (callForCard(cl, dbArgs, cardId)) {
	  if (v>0)
	    fprintf(stderr, "Could not handle card\n");
	}
      }
    }
    else {
      if (v>2)
	fprintf(stderr, "No card (%d)\n", res);
    }
  } /* for */

  return 0;
}


#endif


