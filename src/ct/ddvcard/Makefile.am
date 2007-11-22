INCLUDES=@all_includes@ -I../../ -I../../client -I../../cards

crypttokenplugindir=$(gwenhywfar_plugins)/ct

crypttokenplugin_DATA=ddvcard.xml

crypttokenplugin_LTLIBRARIES=ddvcard.la
noinst_HEADERS=\
  ddvcard_p.h

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif

ddvcard_la_SOURCES=\
  ddvcard.c

ddvcard_la_LDFLAGS=-module -no-undefined
ddvcard_la_LIBADD=../../lib/client/libchipcardc.la $(mingw_ldflags) $(gwenhywfar_libs)


