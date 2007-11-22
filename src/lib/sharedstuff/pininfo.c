/* This file is auto-generated from "pininfo.xml" by the typemaker
   tool of Gwenhywfar. 
   Do not edit this file -- all changes will be lost! */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "pininfo_p.h"
#include <gwenhywfar/misc.h>
#include <gwenhywfar/db.h>
#include <gwenhywfar/debug.h>
#include <assert.h>
#include <stdlib.h>
#include <strings.h>

#include <gwenhywfar/types.h>
#include <chipcard/chipcard.h>


GWEN_INHERIT_FUNCTIONS(LC_PININFO)
GWEN_LIST_FUNCTIONS(LC_PININFO, LC_PinInfo)
GWEN_LIST2_FUNCTIONS(LC_PININFO, LC_PinInfo)



LC_PININFO *LC_PinInfo_new() {
  LC_PININFO *st;

  GWEN_NEW_OBJECT(LC_PININFO, st)
  st->_usage=1;
  GWEN_INHERIT_INIT(LC_PININFO, st)
  GWEN_LIST_INIT(LC_PININFO, st)
  return st;
}


void LC_PinInfo_free(LC_PININFO *st) {
  if (st) {
    assert(st->_usage);
    if (--(st->_usage)==0) {
  GWEN_INHERIT_FINI(LC_PININFO, st)
  if (st->name)
    free(st->name);
  GWEN_LIST_FINI(LC_PININFO, st)
  GWEN_FREE_OBJECT(st);
    }
  }

}


LC_PININFO *LC_PinInfo_dup(const LC_PININFO *d) {
  LC_PININFO *st;

  assert(d);
  st=LC_PinInfo_new();
  if (d->name)
    st->name=strdup(d->name);
  st->id=d->id;
  st->encoding=d->encoding;
  st->minLength=d->minLength;
  st->maxLength=d->maxLength;
  st->allowChange=d->allowChange;
  st->filler=d->filler;
  return st;
}


int LC_PinInfo_toDb(const LC_PININFO *st, GWEN_DB_NODE *db) {
  assert(st);
  assert(db);
  if (st->name)
    if (GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "name", st->name))
      return -1;
  if (GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "id", st->id))
    return -1;
  if (GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "encoding", GWEN_Crypt_PinEncoding_toString(st->encoding)))
    return -1;
  if (GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "minLength", st->minLength))
    return -1;
  if (GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "maxLength", st->maxLength))
    return -1;
  if (GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "allowChange", st->allowChange))
    return -1;
  if (GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "filler", st->filler))
    return -1;
  return 0;
}


int LC_PinInfo_ReadDb(LC_PININFO *st, GWEN_DB_NODE *db) {
  assert(st);
  assert(db);
  LC_PinInfo_SetName(st, GWEN_DB_GetCharValue(db, "name", 0, 0));
  LC_PinInfo_SetId(st, GWEN_DB_GetIntValue(db, "id", 0, 0));
  LC_PinInfo_SetEncoding(st, GWEN_Crypt_PinEncoding_fromString(GWEN_DB_GetCharValue(db, "encoding", 0, 0)));
  LC_PinInfo_SetMinLength(st, GWEN_DB_GetIntValue(db, "minLength", 0, 0));
  LC_PinInfo_SetMaxLength(st, GWEN_DB_GetIntValue(db, "maxLength", 0, 0));
  LC_PinInfo_SetAllowChange(st, GWEN_DB_GetIntValue(db, "allowChange", 0, 0));
  LC_PinInfo_SetFiller(st, GWEN_DB_GetIntValue(db, "filler", 0, 0));
  return 0;
}


LC_PININFO *LC_PinInfo_fromDb(GWEN_DB_NODE *db) {
  LC_PININFO *st;

  assert(db);
  st=LC_PinInfo_new();
  LC_PinInfo_ReadDb(st, db);
  st->_modified=0;
  return st;
}




const char *LC_PinInfo_GetName(const LC_PININFO *st) {
  assert(st);
  return st->name;
}


void LC_PinInfo_SetName(LC_PININFO *st, const char *d) {
  assert(st);
  if (st->name)
    free(st->name);
  if (d && *d)
    st->name=strdup(d);
  else
    st->name=0;
  st->_modified=1;
}




uint32_t LC_PinInfo_GetId(const LC_PININFO *st) {
  assert(st);
  return st->id;
}


void LC_PinInfo_SetId(LC_PININFO *st, uint32_t d) {
  assert(st);
  st->id=d;
  st->_modified=1;
}




GWEN_CRYPT_PINENCODING LC_PinInfo_GetEncoding(const LC_PININFO *st) {
  assert(st);
  return st->encoding;
}


void LC_PinInfo_SetEncoding(LC_PININFO *st, GWEN_CRYPT_PINENCODING d) {
  assert(st);
  st->encoding=d;
  st->_modified=1;
}




int LC_PinInfo_GetMinLength(const LC_PININFO *st) {
  assert(st);
  return st->minLength;
}


void LC_PinInfo_SetMinLength(LC_PININFO *st, int d) {
  assert(st);
  st->minLength=d;
  st->_modified=1;
}




int LC_PinInfo_GetMaxLength(const LC_PININFO *st) {
  assert(st);
  return st->maxLength;
}


void LC_PinInfo_SetMaxLength(LC_PININFO *st, int d) {
  assert(st);
  st->maxLength=d;
  st->_modified=1;
}




int LC_PinInfo_GetAllowChange(const LC_PININFO *st) {
  assert(st);
  return st->allowChange;
}


void LC_PinInfo_SetAllowChange(LC_PININFO *st, int d) {
  assert(st);
  st->allowChange=d;
  st->_modified=1;
}




int LC_PinInfo_GetFiller(const LC_PININFO *st) {
  assert(st);
  return st->filler;
}


void LC_PinInfo_SetFiller(LC_PININFO *st, int d) {
  assert(st);
  st->filler=d;
  st->_modified=1;
}




int LC_PinInfo_IsModified(const LC_PININFO *st) {
  assert(st);
  return st->_modified;
}


void LC_PinInfo_SetModified(LC_PININFO *st, int i) {
  assert(st);
  st->_modified=i;
}


void LC_PinInfo_Attach(LC_PININFO *st) {
  assert(st);
  st->_usage++;
}
LC_PININFO *LC_PinInfo_List2__freeAll_cb(LC_PININFO *st, void *user_data) {
  LC_PinInfo_free(st);
return 0;
}


void LC_PinInfo_List2_freeAll(LC_PININFO_LIST2 *stl) {
  if (stl) {
    LC_PinInfo_List2_ForEach(stl, LC_PinInfo_List2__freeAll_cb, 0);
    LC_PinInfo_List2_free(stl); 
  }
}


LC_PININFO_LIST *LC_PinInfo_List_dup(const LC_PININFO_LIST *stl) {
  if (stl) {
    LC_PININFO_LIST *nl;
    LC_PININFO *e;

    nl=LC_PinInfo_List_new();
    e=LC_PinInfo_List_First(stl);
    while(e) {
      LC_PININFO *ne;

      ne=LC_PinInfo_dup(e);
      assert(ne);
      LC_PinInfo_List_Add(ne, nl);
      e=LC_PinInfo_List_Next(e);
    } /* while (e) */
    return nl;
  }
  else
    return 0;
}




