SUBDIRS=base cards ct PCSC

EXTRA_DIST=

DEFS=-DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)
AM_CPPFLAGS= $(gwenhywfar_CFLAGS) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src
  

lib_LTLIBRARIES=libchipcard.la
noinst_PROGRAMS=libtest


iheaderdir=@libchipcard_headerdir_am@
iheader_HEADERS=\
  chipcard.h \
  version.h

libchipcard_la_SOURCES=dummy.c
libchipcard_la_LIBADD= \
  base/libbase.la \
  cards/libcards.la \
  ct/liblcct.la $(mingw_ldflags) $(gwenhywfar_LIBS) $(zlib_libs) $(pcsc_libraries) $(pcsc_lib)

libchipcard_la_LDFLAGS= \
  @STRIPALL@ -no-undefined -version-info \
  @LIBCHIPCARD_SO_CURRENT@:@LIBCHIPCARD_SO_REVISION@:@LIBCHIPCARD_SO_AGE@


libtest_SOURCES=libtest.c
libtest_LDADD=$(gwenhywfar_LIBS) libchipcard.la


sources:
	for d in $(SUBDIRS); do \
	  $(MAKE) -C $$d sources; \
	done

typefiles:
	for d in $(SUBDIRS); do \
	  $(MAKE) -C $$d typefiles; \
	done

typedefs:
	for d in $(SUBDIRS); do \
	  $(MAKE) -C $$d typedefs; \
	done
