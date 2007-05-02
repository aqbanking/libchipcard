
INCLUDES=@all_includes@ -I../../ -I../../client -I../../cards

crypttokenplugindir=$(gwenhywfar_plugins)/crypttoken

crypttokenplugin_DATA=starcoscard.xml

crypttokenplugin_LTLIBRARIES=starcoscard.la
noinst_HEADERS=\
  starcoscard_p.h

iheaderdir=${includedir}/chipcard2-client/crypttoken
#iheader_HEADERS=

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif

starcoscard_la_SOURCES=\
  starcoscard.c

starcoscard_la_LDFLAGS=-module -no-undefined
starcoscard_la_LIBADD=../../lib/client/libchipcardc.la $(mingw_ldflags) $(gwenhywfar_libs)

