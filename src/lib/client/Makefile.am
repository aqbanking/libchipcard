SUBDIRS=base io cards ct mon

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
  cards/libcards.la \
  ct/liblcct.la \
  mon/libmon.la \
  io/pcsc/libio_pcsc.la \
  ../sharedstuff/libsharedstuff.la $(mingw_ldflags) @gwenhywfar_libs@ @zlib_libs@ @pcsc_libs@

libchipcardc_la_LDFLAGS= \
  @STRIPALL@ -no-undefined -version-info \
  @LIBCHIPCARD_SO_CURRENT@:@LIBCHIPCARD_SO_REVISION@:@LIBCHIPCARD_SO_AGE@


libtest_SOURCES=libtest.c
libtest_LDADD=@gwenhywfar_libs@ libchipcardc.la


