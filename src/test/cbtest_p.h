

#ifndef CBTEST_P_H
#define CBTEST_P_H

#include "cbtest.h"


struct TEST_CALLBACK {
  GWEN_TYPE_UINT64 lastPos;

};


void TEST_Callback_free(void *bp, void *p);

GWEN_WAITCALLBACK *TEST_Callback_Instantiate(GWEN_WAITCALLBACK *ctx);
void TEST_Callback_Log(GWEN_WAITCALLBACK *ctx,
                       unsigned int level,
                       unsigned int logLevel,
                       const char *s);
GWEN_WAITCALLBACK_RESULT
  TEST_Callback_CheckAbort(GWEN_WAITCALLBACK *ctx,
                           unsigned int level);



#endif
