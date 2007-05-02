INCLUDES=@all_includes@

EXTRA_DIST=ressource.rc.in ctapi_reader.c ctapi_secure.c

lib_LTLIBRARIES=libchipcard_ctapi.la

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif


noinst_HEADERS=ctapi.h ctapi_p.h
libchipcard_ctapi_la_SOURCES=ctapi.c
libchipcard_ctapi_la_LDFLAGS=-no-undefined
libchipcard_ctapi_la_LIBADD=\
 ../lib/client/libchipcardc.la $(mingw_ldflags) $(gwenhywfar_libs)

