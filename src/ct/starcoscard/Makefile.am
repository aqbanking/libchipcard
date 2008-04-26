INCLUDES=@all_includes@ -I../../ -I../../client -I../../cards

crypttokenplugindir=$(gwenhywfar_plugins)/ct
crypttokenplugin_DATA=starcoscard.xml

noinst_LTLIBRARIES=libct_starcoscard.la
libct_starcoscard_la_SOURCES=starcoscard.c


crypttokenplugin_LTLIBRARIES=starcoscard.la
noinst_HEADERS=\
  starcoscard_p.h

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif

starcoscard_la_SOURCES=dummy.c

starcoscard_la_LDFLAGS=-module -no-undefined
starcoscard_la_LIBADD=libct_starcoscard.la ../../lib/client/libchipcardc.la $(mingw_ldflags) $(gwenhywfar_libs)


