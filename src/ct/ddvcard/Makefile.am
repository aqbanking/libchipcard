INCLUDES=@all_includes@ -I../../ -I../../client -I../../cards

crypttokenplugindir=$(gwenhywfar_plugins)/ct
crypttokenplugin_DATA=ddvcard.xml

noinst_LTLIBRARIES=libct_ddvcard.la
libct_ddvcard_la_SOURCES=ddvcard.c

crypttokenplugin_LTLIBRARIES=ddvcard.la
noinst_HEADERS=ddvcard_p.h

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif

ddvcard_la_SOURCES=dummy.c

ddvcard_la_LDFLAGS=-module -no-undefined
ddvcard_la_LIBADD=libct_ddvcard.la ../../lib/client/libchipcardc.la $(mingw_ldflags) $(gwenhywfar_libs)


