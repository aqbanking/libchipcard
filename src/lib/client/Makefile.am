SUBDIRS=base io cards ct mon service

EXTRA_DIST=ressource.rc.in

DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)

INCLUDES = $(gwenhywfar_includes)

lib_LTLIBRARIES=libchipcardc.la
noinst_PROGRAMS=libtest

iheaderdir=${includedir}/chipcard/client
iheader_HEADERS=switch.h

libchipcardc_la_SOURCES=switch.c
libchipcardc_la_LIBADD= \
  base/libbase.la \
  io/lcc/libio_lcc.la \
  cards/libcards.la \
  ct/liblcct.la \
  mon/libmon.la \
  service/libservice.la \
  ../sharedstuff/libsharedstuff.la $(mingw_ldflags) @gwenhywfar_libs@ @zlib_libs@

if USE_PCSC
libchipcardc_la_LIBADD+=io/pcsc/libio_pcsc.la @pcsc_libs@
endif

libchipcardc_la_LDFLAGS= \
  @STRIPALL@ -no-undefined -version-info \
  @LIBCHIPCARD_SO_CURRENT@:@LIBCHIPCARD_SO_REVISION@:@LIBCHIPCARD_SO_AGE@


libtest_SOURCES=libtest.c
libtest_LDADD=@gwenhywfar_libs@ libchipcardc.la


