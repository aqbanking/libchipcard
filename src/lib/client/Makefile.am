SUBDIRS=base io cards crypttoken mon service

EXTRA_DIST=ressource.rc.in

DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)

INCLUDES = $(gwenhywfar_includes)

lib_LTLIBRARIES=libchipcard3c.la
noinst_PROGRAMS=libtest

iheaderdir=${includedir}/chipcard3/client
iheader_HEADERS=switch.h

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif

libchipcard3c_la_SOURCES=switch.c
libchipcard3c_la_LIBADD= \
  base/libbase.la \
  io/lcc/libio_lcc.la \
  cards/libcards.la \
  crypttoken/libctcard.la \
  mon/libmon.la \
  service/libservice.la \
  ../sharedstuff/libsharedstuff.la $(mingw_ldflags) @gwenhywfar_libs@

if USE_PCSC
libchipcard3c_la_LIBADD+=io/pcsc/libio_pcsc.la @pcsc_libs@
endif

libchipcard3c_la_LDFLAGS= \
  @STRIPALL@ -no-undefined -version-info \
  @LIBCHIPCARD_SO_CURRENT@:@LIBCHIPCARD_SO_REVISION@:@LIBCHIPCARD_SO_AGE@


libtest_SOURCES=libtest.c
libtest_LDADD=@gwenhywfar_libs@ libchipcard3c.la


