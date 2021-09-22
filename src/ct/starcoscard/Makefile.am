AM_CPPFLAGS= $(gwenhywfar_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

crypttokenplugindir=$(gwenhywfar_plugins)/ct
crypttokenplugin_DATA=starcoscard.xml

noinst_LTLIBRARIES=libct_starcoscard.la
libct_starcoscard_la_SOURCES=starcoscard.c


crypttokenplugin_LTLIBRARIES=starcoscard.la
noinst_HEADERS=\
  starcoscard_p.h

starcoscard_la_SOURCES=dummy.c

starcoscard_la_LDFLAGS=-module -no-undefined -avoid-version
starcoscard_la_LIBADD=libct_starcoscard.la ../../libchipcard/libchipcard.la $(mingw_ldflags) $(gwenhywfar_libs)


sources:
	for f in $(libct_starcoscard_la_SOURCES) $(extra_sources); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done

typefiles:

typedefs:
