INCLUDES=@all_includes@ -I../../ -I../../client -I../../cards

crypttokenplugindir=$(gwenhywfar_plugins)/crypttoken

crypttokenplugin_DATA=ddvcard.xml

crypttokenplugin_LTLIBRARIES=ddvcard.la
noinst_HEADERS=\
  ddvcard_p.h

iheaderdir=${includedir}/chipcard3/client/crypttoken
#iheader_HEADERS=

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif

ddvcard_la_SOURCES=\
  ddvcard.c

ddvcard_la_LDFLAGS=-module -no-undefined
ddvcard_la_LIBADD=../../lib/client/libchipcard3c.la $(mingw_ldflags) $(gwenhywfar_libs)


