/* This file is auto-generated from "starcos_keydescr.xml" by the typemaker
   tool of Gwenhywfar. 
   Do not edit this file -- all changes will be lost! */
#ifndef STARCOS_KEYDESCR_H
#define STARCOS_KEYDESCR_H

/** @page P_LC_STARCOS_KEYDESCR_PUBLIC LC_Starcos_KeyDescr (public)
This page describes the properties of LC_STARCOS_KEYDESCR
@anchor LC_STARCOS_KEYDESCR_KeyId
<h3>KeyId</h3>
<p>
</p>
<p>
Set this property with @ref LC_Starcos_KeyDescr_SetKeyId, 
get it with @ref LC_Starcos_KeyDescr_GetKeyId
</p>

@anchor LC_STARCOS_KEYDESCR_Status
<h3>Status</h3>
<p>
</p>
<p>
Set this property with @ref LC_Starcos_KeyDescr_SetStatus, 
get it with @ref LC_Starcos_KeyDescr_GetStatus
</p>

@anchor LC_STARCOS_KEYDESCR_KeyType
<h3>KeyType</h3>
<p>
</p>
<p>
Set this property with @ref LC_Starcos_KeyDescr_SetKeyType, 
get it with @ref LC_Starcos_KeyDescr_GetKeyType
</p>

@anchor LC_STARCOS_KEYDESCR_KeyNum
<h3>KeyNum</h3>
<p>
</p>
<p>
Set this property with @ref LC_Starcos_KeyDescr_SetKeyNum, 
get it with @ref LC_Starcos_KeyDescr_GetKeyNum
</p>

@anchor LC_STARCOS_KEYDESCR_KeyVer
<h3>KeyVer</h3>
<p>
</p>
<p>
Set this property with @ref LC_Starcos_KeyDescr_SetKeyVer, 
get it with @ref LC_Starcos_KeyDescr_GetKeyVer
</p>

*/
#ifdef __cplusplus
extern "C" {
#endif

typedef struct LC_STARCOS_KEYDESCR LC_STARCOS_KEYDESCR;

#ifdef __cplusplus
} /* __cplusplus */
#endif

#include <gwenhywfar/db.h>
/* headers */
#include <chipcard2-client/cards/starcos.h>
#include <chipcard2/chipcard2.h>

#ifdef __cplusplus
extern "C" {
#endif


/** Creates a new object.
*/
CHIPCARD_API LC_STARCOS_KEYDESCR *LC_Starcos_KeyDescr_new();
/** Creates an object from the data in the given GWEN_DB_NODE
*/
CHIPCARD_API LC_STARCOS_KEYDESCR *LC_Starcos_KeyDescr_fromDb(GWEN_DB_NODE *db);
/** Creates and returns a deep copy of thegiven object.
*/
CHIPCARD_API LC_STARCOS_KEYDESCR *LC_Starcos_KeyDescr_dup(const LC_STARCOS_KEYDESCR*st);
/** Destroys the given object.
*/
CHIPCARD_API void LC_Starcos_KeyDescr_free(LC_STARCOS_KEYDESCR *st);
/** Increments the usage counter of the given object, so an additional free() is needed to destroy the object.
*/
CHIPCARD_API void LC_Starcos_KeyDescr_Attach(LC_STARCOS_KEYDESCR *st);
/** Reads data from a GWEN_DB.
*/
CHIPCARD_API int LC_Starcos_KeyDescr_ReadDb(LC_STARCOS_KEYDESCR *st, GWEN_DB_NODE *db);
/** Stores an object in the given GWEN_DB_NODE
*/
CHIPCARD_API int LC_Starcos_KeyDescr_toDb(const LC_STARCOS_KEYDESCR*st, GWEN_DB_NODE *db);
/** Returns 0 if this object has not been modified, !=0 otherwise
*/
CHIPCARD_API int LC_Starcos_KeyDescr_IsModified(const LC_STARCOS_KEYDESCR *st);
/** Sets the modified state of the given object
*/
CHIPCARD_API void LC_Starcos_KeyDescr_SetModified(LC_STARCOS_KEYDESCR *st, int i);

/**
* Returns the property @ref LC_STARCOS_KEYDESCR_KeyId
*/
CHIPCARD_API int LC_Starcos_KeyDescr_GetKeyId(const LC_STARCOS_KEYDESCR *el);
/**
* Set the property @ref LC_STARCOS_KEYDESCR_KeyId
*/
CHIPCARD_API void LC_Starcos_KeyDescr_SetKeyId(LC_STARCOS_KEYDESCR *el, int d);

/**
* Returns the property @ref LC_STARCOS_KEYDESCR_Status
*/
CHIPCARD_API int LC_Starcos_KeyDescr_GetStatus(const LC_STARCOS_KEYDESCR *el);
/**
* Set the property @ref LC_STARCOS_KEYDESCR_Status
*/
CHIPCARD_API void LC_Starcos_KeyDescr_SetStatus(LC_STARCOS_KEYDESCR *el, int d);

/**
* Returns the property @ref LC_STARCOS_KEYDESCR_KeyType
*/
CHIPCARD_API int LC_Starcos_KeyDescr_GetKeyType(const LC_STARCOS_KEYDESCR *el);
/**
* Set the property @ref LC_STARCOS_KEYDESCR_KeyType
*/
CHIPCARD_API void LC_Starcos_KeyDescr_SetKeyType(LC_STARCOS_KEYDESCR *el, int d);

/**
* Returns the property @ref LC_STARCOS_KEYDESCR_KeyNum
*/
CHIPCARD_API int LC_Starcos_KeyDescr_GetKeyNum(const LC_STARCOS_KEYDESCR *el);
/**
* Set the property @ref LC_STARCOS_KEYDESCR_KeyNum
*/
CHIPCARD_API void LC_Starcos_KeyDescr_SetKeyNum(LC_STARCOS_KEYDESCR *el, int d);

/**
* Returns the property @ref LC_STARCOS_KEYDESCR_KeyVer
*/
CHIPCARD_API int LC_Starcos_KeyDescr_GetKeyVer(const LC_STARCOS_KEYDESCR *el);
/**
* Set the property @ref LC_STARCOS_KEYDESCR_KeyVer
*/
CHIPCARD_API void LC_Starcos_KeyDescr_SetKeyVer(LC_STARCOS_KEYDESCR *el, int d);


#ifdef __cplusplus
} /* __cplusplus */
#endif


#endif /* STARCOS_KEYDESCR_H */
