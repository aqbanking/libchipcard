/* This file is auto-generated from "geldkarte_blog.xml" by the typemaker
   tool of Gwenhywfar. 
   Do not edit this file -- all changes will be lost! */
#ifndef GELDKARTE_BLOG_H
#define GELDKARTE_BLOG_H

/** @page P_LC_GELDKARTE_BLOG_PUBLIC LC_GeldKarte_BLog (public)
This page describes the properties of LC_GELDKARTE_BLOG
@anchor LC_GELDKARTE_BLOG_Status
<h3>Status</h3>
<p>
</p>
<p>
Set this property with @ref LC_GeldKarte_BLog_SetStatus, 
get it with @ref LC_GeldKarte_BLog_GetStatus
</p>

@anchor LC_GELDKARTE_BLOG_BSeq
<h3>BSeq</h3>
<p>
</p>
<p>
Set this property with @ref LC_GeldKarte_BLog_SetBSeq, 
get it with @ref LC_GeldKarte_BLog_GetBSeq
</p>

@anchor LC_GELDKARTE_BLOG_LSeq
<h3>LSeq</h3>
<p>
</p>
<p>
Set this property with @ref LC_GeldKarte_BLog_SetLSeq, 
get it with @ref LC_GeldKarte_BLog_GetLSeq
</p>

@anchor LC_GELDKARTE_BLOG_Value
<h3>Value</h3>
<p>
</p>
<p>
Set this property with @ref LC_GeldKarte_BLog_SetValue, 
get it with @ref LC_GeldKarte_BLog_GetValue
</p>

@anchor LC_GELDKARTE_BLOG_MerchantId
<h3>MerchantId</h3>
<p>
</p>
<p>
Set this property with @ref LC_GeldKarte_BLog_SetMerchantId, 
get it with @ref LC_GeldKarte_BLog_GetMerchantId
</p>

@anchor LC_GELDKARTE_BLOG_HSeq
<h3>HSeq</h3>
<p>
</p>
<p>
Set this property with @ref LC_GeldKarte_BLog_SetHSeq, 
get it with @ref LC_GeldKarte_BLog_GetHSeq
</p>

@anchor LC_GELDKARTE_BLOG_SSeq
<h3>SSeq</h3>
<p>
</p>
<p>
Set this property with @ref LC_GeldKarte_BLog_SetSSeq, 
get it with @ref LC_GeldKarte_BLog_GetSSeq
</p>

@anchor LC_GELDKARTE_BLOG_Loaded
<h3>Loaded</h3>
<p>
</p>
<p>
Set this property with @ref LC_GeldKarte_BLog_SetLoaded, 
get it with @ref LC_GeldKarte_BLog_GetLoaded
</p>

@anchor LC_GELDKARTE_BLOG_Time
<h3>Time</h3>
<p>
</p>
<p>
Set this property with @ref LC_GeldKarte_BLog_SetTime, 
get it with @ref LC_GeldKarte_BLog_GetTime
</p>

@anchor LC_GELDKARTE_BLOG_KeyId
<h3>KeyId</h3>
<p>
</p>
<p>
Set this property with @ref LC_GeldKarte_BLog_SetKeyId, 
get it with @ref LC_GeldKarte_BLog_GetKeyId
</p>

*/
#ifdef __cplusplus
extern "C" {
#endif

typedef struct LC_GELDKARTE_BLOG LC_GELDKARTE_BLOG;

#ifdef __cplusplus
} /* __cplusplus */
#endif

#include <gwenhywfar/db.h>
#include <gwenhywfar/list2.h>
/* headers */
#include <gwenhywfar/types.h>
#include <gwenhywfar/gwentime.h>
#include <chipcard3/chipcard3.h>

#ifdef __cplusplus
extern "C" {
#endif


GWEN_LIST2_FUNCTION_LIB_DEFS(LC_GELDKARTE_BLOG, LC_GeldKarte_BLog, CHIPCARD_API)

/** Destroys all objects stored in the given LIST2 and the list itself
*/
CHIPCARD_API void LC_GeldKarte_BLog_List2_freeAll(LC_GELDKARTE_BLOG_LIST2 *stl);

/** Creates a new object.
*/
CHIPCARD_API LC_GELDKARTE_BLOG *LC_GeldKarte_BLog_new();
/** Creates an object from the data in the given GWEN_DB_NODE
*/
CHIPCARD_API LC_GELDKARTE_BLOG *LC_GeldKarte_BLog_fromDb(GWEN_DB_NODE *db);
/** Creates and returns a deep copy of thegiven object.
*/
CHIPCARD_API LC_GELDKARTE_BLOG *LC_GeldKarte_BLog_dup(const LC_GELDKARTE_BLOG*st);
/** Destroys the given object.
*/
CHIPCARD_API void LC_GeldKarte_BLog_free(LC_GELDKARTE_BLOG *st);
/** Increments the usage counter of the given object, so an additional free() is needed to destroy the object.
*/
CHIPCARD_API void LC_GeldKarte_BLog_Attach(LC_GELDKARTE_BLOG *st);
/** Reads data from a GWEN_DB.
*/
CHIPCARD_API int LC_GeldKarte_BLog_ReadDb(LC_GELDKARTE_BLOG *st, GWEN_DB_NODE *db);
/** Stores an object in the given GWEN_DB_NODE
*/
CHIPCARD_API int LC_GeldKarte_BLog_toDb(const LC_GELDKARTE_BLOG*st, GWEN_DB_NODE *db);
/** Returns 0 if this object has not been modified, !=0 otherwise
*/
CHIPCARD_API int LC_GeldKarte_BLog_IsModified(const LC_GELDKARTE_BLOG *st);
/** Sets the modified state of the given object
*/
CHIPCARD_API void LC_GeldKarte_BLog_SetModified(LC_GELDKARTE_BLOG *st, int i);

/**
* Returns the property @ref LC_GELDKARTE_BLOG_Status
*/
CHIPCARD_API int LC_GeldKarte_BLog_GetStatus(const LC_GELDKARTE_BLOG *el);
/**
* Set the property @ref LC_GELDKARTE_BLOG_Status
*/
CHIPCARD_API void LC_GeldKarte_BLog_SetStatus(LC_GELDKARTE_BLOG *el, int d);

/**
* Returns the property @ref LC_GELDKARTE_BLOG_BSeq
*/
CHIPCARD_API int LC_GeldKarte_BLog_GetBSeq(const LC_GELDKARTE_BLOG *el);
/**
* Set the property @ref LC_GELDKARTE_BLOG_BSeq
*/
CHIPCARD_API void LC_GeldKarte_BLog_SetBSeq(LC_GELDKARTE_BLOG *el, int d);

/**
* Returns the property @ref LC_GELDKARTE_BLOG_LSeq
*/
CHIPCARD_API int LC_GeldKarte_BLog_GetLSeq(const LC_GELDKARTE_BLOG *el);
/**
* Set the property @ref LC_GELDKARTE_BLOG_LSeq
*/
CHIPCARD_API void LC_GeldKarte_BLog_SetLSeq(LC_GELDKARTE_BLOG *el, int d);

/**
* Returns the property @ref LC_GELDKARTE_BLOG_Value
*/
CHIPCARD_API int LC_GeldKarte_BLog_GetValue(const LC_GELDKARTE_BLOG *el);
/**
* Set the property @ref LC_GELDKARTE_BLOG_Value
*/
CHIPCARD_API void LC_GeldKarte_BLog_SetValue(LC_GELDKARTE_BLOG *el, int d);

/**
* Returns the property @ref LC_GELDKARTE_BLOG_MerchantId
*/
CHIPCARD_API const char *LC_GeldKarte_BLog_GetMerchantId(const LC_GELDKARTE_BLOG *el);
/**
* Set the property @ref LC_GELDKARTE_BLOG_MerchantId
*/
CHIPCARD_API void LC_GeldKarte_BLog_SetMerchantId(LC_GELDKARTE_BLOG *el, const char *d);

/**
* Returns the property @ref LC_GELDKARTE_BLOG_HSeq
*/
CHIPCARD_API int LC_GeldKarte_BLog_GetHSeq(const LC_GELDKARTE_BLOG *el);
/**
* Set the property @ref LC_GELDKARTE_BLOG_HSeq
*/
CHIPCARD_API void LC_GeldKarte_BLog_SetHSeq(LC_GELDKARTE_BLOG *el, int d);

/**
* Returns the property @ref LC_GELDKARTE_BLOG_SSeq
*/
CHIPCARD_API int LC_GeldKarte_BLog_GetSSeq(const LC_GELDKARTE_BLOG *el);
/**
* Set the property @ref LC_GELDKARTE_BLOG_SSeq
*/
CHIPCARD_API void LC_GeldKarte_BLog_SetSSeq(LC_GELDKARTE_BLOG *el, int d);

/**
* Returns the property @ref LC_GELDKARTE_BLOG_Loaded
*/
CHIPCARD_API int LC_GeldKarte_BLog_GetLoaded(const LC_GELDKARTE_BLOG *el);
/**
* Set the property @ref LC_GELDKARTE_BLOG_Loaded
*/
CHIPCARD_API void LC_GeldKarte_BLog_SetLoaded(LC_GELDKARTE_BLOG *el, int d);

/**
* Returns the property @ref LC_GELDKARTE_BLOG_Time
*/
CHIPCARD_API const GWEN_TIME *LC_GeldKarte_BLog_GetTime(const LC_GELDKARTE_BLOG *el);
/**
* Set the property @ref LC_GELDKARTE_BLOG_Time
*/
CHIPCARD_API void LC_GeldKarte_BLog_SetTime(LC_GELDKARTE_BLOG *el, const GWEN_TIME *d);

/**
* Returns the property @ref LC_GELDKARTE_BLOG_KeyId
*/
CHIPCARD_API int LC_GeldKarte_BLog_GetKeyId(const LC_GELDKARTE_BLOG *el);
/**
* Set the property @ref LC_GELDKARTE_BLOG_KeyId
*/
CHIPCARD_API void LC_GeldKarte_BLog_SetKeyId(LC_GELDKARTE_BLOG *el, int d);


#ifdef __cplusplus
} /* __cplusplus */
#endif


#endif /* GELDKARTE_BLOG_H */