

#define GWEN_EXTEND_WAITCALLBACK

#include <gwenhywfar/debug.h>

#include "cbtest_p.h"

GWEN_INHERIT(GWEN_WAITCALLBACK, TEST_CALLBACK);


GWEN_WAITCALLBACK *TEST_Callback_new(const char *id){
  GWEN_WAITCALLBACK *ctx;
  TEST_CALLBACK *cb;

  ctx=GWEN_WaitCallback_new(id);
  cb=(TEST_CALLBACK*)malloc(sizeof(TEST_CALLBACK));
  memset(cb, 0, sizeof(TEST_CALLBACK));
  GWEN_INHERIT_SETDATA(GWEN_WAITCALLBACK, TEST_CALLBACK,
                       ctx, cb,
                       TEST_Callback_free);
  GWEN_WaitCallback_SetCheckAbortFn(ctx, TEST_Callback_CheckAbort);
  GWEN_WaitCallback_SetInstantiateFn(ctx, TEST_Callback_Instantiate);
  GWEN_WaitCallback_SetLogFn(ctx, TEST_Callback_Log);
  return ctx;
}



void TEST_Callback_free(void *bp, void *p){
  GWEN_WAITCALLBACK *ctx;
  TEST_CALLBACK *cb;

  ctx=(GWEN_WAITCALLBACK*)bp;
  cb=(TEST_CALLBACK*)p;

  free(cb);
}



GWEN_WAITCALLBACK *TEST_Callback_Instantiate(GWEN_WAITCALLBACK *ctx){
  GWEN_WAITCALLBACK *nctx;

  assert(ctx);
  nctx=TEST_Callback_new(GWEN_WaitCallback_GetId(ctx));

  DBG_INFO(0, "Instantiating callback \"%s\"",
           GWEN_WaitCallback_GetId(ctx));
  GWEN_WaitCallback_SetDistance(nctx, GWEN_WaitCallback_GetDistance(ctx));
  return nctx;
}



void TEST_Callback_Log(GWEN_WAITCALLBACK *ctx,
                       unsigned int level,
                       unsigned int logLevel,
                       const char *s){
  TEST_CALLBACK *cb;

  cb=GWEN_INHERIT_GETDATA(GWEN_WAITCALLBACK, TEST_CALLBACK, ctx);
  assert(cb);

  DBG_NOTICE(0, "Callback log: %s", s);
}



GWEN_WAITCALLBACK_RESULT
TEST_Callback_CheckAbort(GWEN_WAITCALLBACK *ctx,
                         unsigned int level){
  TEST_CALLBACK *cb;
  GWEN_TYPE_UINT64 pos;

  cb=GWEN_INHERIT_GETDATA(GWEN_WAITCALLBACK, TEST_CALLBACK, ctx);
  assert(cb);

  pos=GWEN_WaitCallback_GetProgressPos(ctx);
  if (cb->lastPos!=pos || cb->lastPos==0) {
    DBG_DEBUG(0,
              "Progress: %qd of %qd",
              pos, GWEN_WaitCallback_GetProgressTotal(ctx));
    cb->lastPos=pos;
  }
  return GWEN_WaitCallbackResult_Continue;
}




