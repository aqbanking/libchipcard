/* This file is auto-generated from "hipersonaldata.xml" by the typemaker
   tool of Gwenhywfar.
   Do not edit this file -- all changes will be lost! */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "hipersonaldata_p.h"
#include <gwenhywfar/misc.h>
#include <gwenhywfar/db.h>
#include <gwenhywfar/debug.h>
#include <assert.h>
#include <stdlib.h>
#include <strings.h>

#include <chipcard/chipcard.h>
#include <gwenhywfar/gwentime.h>


GWEN_LIST_FUNCTIONS(LC_HI_PERSONAL_DATA, LC_HIPersonalData)
GWEN_LIST2_FUNCTIONS(LC_HI_PERSONAL_DATA, LC_HIPersonalData)


LC_HI_PERSONAL_DATA_SEX LC_HIPersonalData_Sex_fromString(const char *s)
{
  if (s) {
    if (strcasecmp(s, "male")==0)
      return LC_HIPersonalData_SexMale;
    else if (strcasecmp(s, "female")==0)
      return LC_HIPersonalData_SexFemale;
  }
  return LC_HIPersonalData_SexUnknown;
}


const char *LC_HIPersonalData_Sex_toString(LC_HI_PERSONAL_DATA_SEX v)
{
  switch (v) {
  case LC_HIPersonalData_SexMale:
    return "male";

  case LC_HIPersonalData_SexFemale:
    return "female";

  default:
    return "unknown";
  } /* switch */
}




LC_HI_PERSONAL_DATA *LC_HIPersonalData_new()
{
  LC_HI_PERSONAL_DATA *st;

  GWEN_NEW_OBJECT(LC_HI_PERSONAL_DATA, st)
  st->_usage=1;
  GWEN_LIST_INIT(LC_HI_PERSONAL_DATA, st)
  return st;
}


void LC_HIPersonalData_free(LC_HI_PERSONAL_DATA *st)
{
  if (st) {
    assert(st->_usage);
    if (--(st->_usage)==0) {
      if (st->insuranceId)
        free(st->insuranceId);
      if (st->prename)
        free(st->prename);
      if (st->name)
        free(st->name);
      if (st->title)
        free(st->title);
      if (st->nameSuffix)
        free(st->nameSuffix);
      if (st->dateOfBirth)
        GWEN_Time_free(st->dateOfBirth);
      if (st->addrZipCode)
        free(st->addrZipCode);
      if (st->addrCity)
        free(st->addrCity);
      if (st->addrState)
        free(st->addrState);
      if (st->addrCountry)
        free(st->addrCountry);
      if (st->addrStreet)
        free(st->addrStreet);
      if (st->addrHouseNum)
        free(st->addrHouseNum);
      GWEN_LIST_FINI(LC_HI_PERSONAL_DATA, st)
      GWEN_FREE_OBJECT(st);
    }
  }

}


LC_HI_PERSONAL_DATA *LC_HIPersonalData_dup(const LC_HI_PERSONAL_DATA *d)
{
  LC_HI_PERSONAL_DATA *st;

  assert(d);
  st=LC_HIPersonalData_new();
  if (d->insuranceId)
    st->insuranceId=strdup(d->insuranceId);
  if (d->prename)
    st->prename=strdup(d->prename);
  if (d->name)
    st->name=strdup(d->name);
  if (d->title)
    st->title=strdup(d->title);
  if (d->nameSuffix)
    st->nameSuffix=strdup(d->nameSuffix);
  st->sex=d->sex;
  if (d->dateOfBirth)
    st->dateOfBirth=GWEN_Time_dup(d->dateOfBirth);
  if (d->addrZipCode)
    st->addrZipCode=strdup(d->addrZipCode);
  if (d->addrCity)
    st->addrCity=strdup(d->addrCity);
  if (d->addrState)
    st->addrState=strdup(d->addrState);
  if (d->addrCountry)
    st->addrCountry=strdup(d->addrCountry);
  if (d->addrStreet)
    st->addrStreet=strdup(d->addrStreet);
  if (d->addrHouseNum)
    st->addrHouseNum=strdup(d->addrHouseNum);
  return st;
}


int LC_HIPersonalData_toDb(const LC_HI_PERSONAL_DATA *st, GWEN_DB_NODE *db)
{
  assert(st);
  assert(db);
  if (st->insuranceId)
    if (GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "insuranceId", st->insuranceId))
      return -1;
  if (st->prename)
    if (GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "prename", st->prename))
      return -1;
  if (st->name)
    if (GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "name", st->name))
      return -1;
  if (st->title)
    if (GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "title", st->title))
      return -1;
  if (st->nameSuffix)
    if (GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "nameSuffix", st->nameSuffix))
      return -1;
  if (GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "sex", LC_HIPersonalData_Sex_toString(st->sex)))
    return -1;
  if (st->dateOfBirth)
    if (GWEN_Time_toDb(st->dateOfBirth, GWEN_DB_GetGroup(db, GWEN_DB_FLAGS_DEFAULT, "dateOfBirth")))
      return -1;
  if (st->addrZipCode)
    if (GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "addrZipCode", st->addrZipCode))
      return -1;
  if (st->addrCity)
    if (GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "addrCity", st->addrCity))
      return -1;
  if (st->addrState)
    if (GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "addrState", st->addrState))
      return -1;
  if (st->addrCountry)
    if (GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "addrCountry", st->addrCountry))
      return -1;
  if (st->addrStreet)
    if (GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "addrStreet", st->addrStreet))
      return -1;
  if (st->addrHouseNum)
    if (GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "addrHouseNum", st->addrHouseNum))
      return -1;
  return 0;
}


int LC_HIPersonalData_ReadDb(LC_HI_PERSONAL_DATA *st, GWEN_DB_NODE *db)
{
  assert(st);
  assert(db);
  LC_HIPersonalData_SetInsuranceId(st, GWEN_DB_GetCharValue(db, "insuranceId", 0, 0));
  LC_HIPersonalData_SetPrename(st, GWEN_DB_GetCharValue(db, "prename", 0, 0));
  LC_HIPersonalData_SetName(st, GWEN_DB_GetCharValue(db, "name", 0, 0));
  LC_HIPersonalData_SetTitle(st, GWEN_DB_GetCharValue(db, "title", 0, 0));
  LC_HIPersonalData_SetNameSuffix(st, GWEN_DB_GetCharValue(db, "nameSuffix", 0, 0));
  LC_HIPersonalData_SetSex(st, LC_HIPersonalData_Sex_fromString(GWEN_DB_GetCharValue(db, "sex", 0, 0)));
  if (1) { /* for local vars */
    GWEN_DB_NODE *dbT;

    dbT=GWEN_DB_GetGroup(db, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "dateOfBirth");
    if (dbT) {
      if (st->dateOfBirth)
        GWEN_Time_free(st->dateOfBirth);
      st->dateOfBirth=GWEN_Time_fromDb(dbT);
    }
  }
  LC_HIPersonalData_SetAddrZipCode(st, GWEN_DB_GetCharValue(db, "addrZipCode", 0, 0));
  LC_HIPersonalData_SetAddrCity(st, GWEN_DB_GetCharValue(db, "addrCity", 0, 0));
  LC_HIPersonalData_SetAddrState(st, GWEN_DB_GetCharValue(db, "addrState", 0, 0));
  LC_HIPersonalData_SetAddrCountry(st, GWEN_DB_GetCharValue(db, "addrCountry", 0, 0));
  LC_HIPersonalData_SetAddrStreet(st, GWEN_DB_GetCharValue(db, "addrStreet", 0, 0));
  LC_HIPersonalData_SetAddrHouseNum(st, GWEN_DB_GetCharValue(db, "addrHouseNum", 0, 0));
  return 0;
}


LC_HI_PERSONAL_DATA *LC_HIPersonalData_fromDb(GWEN_DB_NODE *db)
{
  LC_HI_PERSONAL_DATA *st;

  assert(db);
  st=LC_HIPersonalData_new();
  LC_HIPersonalData_ReadDb(st, db);
  st->_modified=0;
  return st;
}




const char *LC_HIPersonalData_GetInsuranceId(const LC_HI_PERSONAL_DATA *st)
{
  assert(st);
  return st->insuranceId;
}


void LC_HIPersonalData_SetInsuranceId(LC_HI_PERSONAL_DATA *st, const char *d)
{
  assert(st);
  if (st->insuranceId)
    free(st->insuranceId);
  if (d && *d)
    st->insuranceId=strdup(d);
  else
    st->insuranceId=0;
  st->_modified=1;
}




const char *LC_HIPersonalData_GetPrename(const LC_HI_PERSONAL_DATA *st)
{
  assert(st);
  return st->prename;
}


void LC_HIPersonalData_SetPrename(LC_HI_PERSONAL_DATA *st, const char *d)
{
  assert(st);
  if (st->prename)
    free(st->prename);
  if (d && *d)
    st->prename=strdup(d);
  else
    st->prename=0;
  st->_modified=1;
}




const char *LC_HIPersonalData_GetName(const LC_HI_PERSONAL_DATA *st)
{
  assert(st);
  return st->name;
}


void LC_HIPersonalData_SetName(LC_HI_PERSONAL_DATA *st, const char *d)
{
  assert(st);
  if (st->name)
    free(st->name);
  if (d && *d)
    st->name=strdup(d);
  else
    st->name=0;
  st->_modified=1;
}




const char *LC_HIPersonalData_GetTitle(const LC_HI_PERSONAL_DATA *st)
{
  assert(st);
  return st->title;
}


void LC_HIPersonalData_SetTitle(LC_HI_PERSONAL_DATA *st, const char *d)
{
  assert(st);
  if (st->title)
    free(st->title);
  if (d && *d)
    st->title=strdup(d);
  else
    st->title=0;
  st->_modified=1;
}




const char *LC_HIPersonalData_GetNameSuffix(const LC_HI_PERSONAL_DATA *st)
{
  assert(st);
  return st->nameSuffix;
}


void LC_HIPersonalData_SetNameSuffix(LC_HI_PERSONAL_DATA *st, const char *d)
{
  assert(st);
  if (st->nameSuffix)
    free(st->nameSuffix);
  if (d && *d)
    st->nameSuffix=strdup(d);
  else
    st->nameSuffix=0;
  st->_modified=1;
}




LC_HI_PERSONAL_DATA_SEX LC_HIPersonalData_GetSex(const LC_HI_PERSONAL_DATA *st)
{
  assert(st);
  return st->sex;
}


void LC_HIPersonalData_SetSex(LC_HI_PERSONAL_DATA *st, LC_HI_PERSONAL_DATA_SEX d)
{
  assert(st);
  st->sex=d;
  st->_modified=1;
}




const GWEN_TIME *LC_HIPersonalData_GetDateOfBirth(const LC_HI_PERSONAL_DATA *st)
{
  assert(st);
  return st->dateOfBirth;
}


void LC_HIPersonalData_SetDateOfBirth(LC_HI_PERSONAL_DATA *st, const GWEN_TIME *d)
{
  assert(st);
  if (st->dateOfBirth)
    GWEN_Time_free(st->dateOfBirth);
  if (d)
    st->dateOfBirth=GWEN_Time_dup(d);
  else
    st->dateOfBirth=0;
  st->_modified=1;
}




const char *LC_HIPersonalData_GetAddrZipCode(const LC_HI_PERSONAL_DATA *st)
{
  assert(st);
  return st->addrZipCode;
}


void LC_HIPersonalData_SetAddrZipCode(LC_HI_PERSONAL_DATA *st, const char *d)
{
  assert(st);
  if (st->addrZipCode)
    free(st->addrZipCode);
  if (d && *d)
    st->addrZipCode=strdup(d);
  else
    st->addrZipCode=0;
  st->_modified=1;
}




const char *LC_HIPersonalData_GetAddrCity(const LC_HI_PERSONAL_DATA *st)
{
  assert(st);
  return st->addrCity;
}


void LC_HIPersonalData_SetAddrCity(LC_HI_PERSONAL_DATA *st, const char *d)
{
  assert(st);
  if (st->addrCity)
    free(st->addrCity);
  if (d && *d)
    st->addrCity=strdup(d);
  else
    st->addrCity=0;
  st->_modified=1;
}




const char *LC_HIPersonalData_GetAddrState(const LC_HI_PERSONAL_DATA *st)
{
  assert(st);
  return st->addrState;
}


void LC_HIPersonalData_SetAddrState(LC_HI_PERSONAL_DATA *st, const char *d)
{
  assert(st);
  if (st->addrState)
    free(st->addrState);
  if (d && *d)
    st->addrState=strdup(d);
  else
    st->addrState=0;
  st->_modified=1;
}




const char *LC_HIPersonalData_GetAddrCountry(const LC_HI_PERSONAL_DATA *st)
{
  assert(st);
  return st->addrCountry;
}


void LC_HIPersonalData_SetAddrCountry(LC_HI_PERSONAL_DATA *st, const char *d)
{
  assert(st);
  if (st->addrCountry)
    free(st->addrCountry);
  if (d && *d)
    st->addrCountry=strdup(d);
  else
    st->addrCountry=0;
  st->_modified=1;
}




const char *LC_HIPersonalData_GetAddrStreet(const LC_HI_PERSONAL_DATA *st)
{
  assert(st);
  return st->addrStreet;
}


void LC_HIPersonalData_SetAddrStreet(LC_HI_PERSONAL_DATA *st, const char *d)
{
  assert(st);
  if (st->addrStreet)
    free(st->addrStreet);
  if (d && *d)
    st->addrStreet=strdup(d);
  else
    st->addrStreet=0;
  st->_modified=1;
}




const char *LC_HIPersonalData_GetAddrHouseNum(const LC_HI_PERSONAL_DATA *st)
{
  assert(st);
  return st->addrHouseNum;
}


void LC_HIPersonalData_SetAddrHouseNum(LC_HI_PERSONAL_DATA *st, const char *d)
{
  assert(st);
  if (st->addrHouseNum)
    free(st->addrHouseNum);
  if (d && *d)
    st->addrHouseNum=strdup(d);
  else
    st->addrHouseNum=0;
  st->_modified=1;
}




int LC_HIPersonalData_IsModified(const LC_HI_PERSONAL_DATA *st)
{
  assert(st);
  return st->_modified;
}


void LC_HIPersonalData_SetModified(LC_HI_PERSONAL_DATA *st, int i)
{
  assert(st);
  st->_modified=i;
}


void LC_HIPersonalData_Attach(LC_HI_PERSONAL_DATA *st)
{
  assert(st);
  st->_usage++;
}
LC_HI_PERSONAL_DATA *LC_HIPersonalData_List2__freeAll_cb(LC_HI_PERSONAL_DATA *st, void *user_data)
{
  LC_HIPersonalData_free(st);
  return 0;
}


void LC_HIPersonalData_List2_freeAll(LC_HI_PERSONAL_DATA_LIST2 *stl)
{
  if (stl) {
    LC_HIPersonalData_List2_ForEach(stl, LC_HIPersonalData_List2__freeAll_cb, 0);
    LC_HIPersonalData_List2_free(stl);
  }
}


LC_HI_PERSONAL_DATA_LIST *LC_HIPersonalData_List_dup(const LC_HI_PERSONAL_DATA_LIST *stl)
{
  if (stl) {
    LC_HI_PERSONAL_DATA_LIST *nl;
    LC_HI_PERSONAL_DATA *e;

    nl=LC_HIPersonalData_List_new();
    e=LC_HIPersonalData_List_First(stl);
    while (e) {
      LC_HI_PERSONAL_DATA *ne;

      ne=LC_HIPersonalData_dup(e);
      assert(ne);
      LC_HIPersonalData_List_Add(ne, nl);
      e=LC_HIPersonalData_List_Next(e);
    } /* while (e) */
    return nl;
  }
  else
    return 0;
}




