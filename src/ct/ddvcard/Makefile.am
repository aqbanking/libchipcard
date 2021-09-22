AM_CPPFLAGS= $(gwenhywfar_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

crypttokenplugindir=$(gwenhywfar_plugins)/ct
crypttokenplugin_DATA=ddvcard.xml

noinst_LTLIBRARIES=libct_ddvcard.la
libct_ddvcard_la_SOURCES=ddvcard.c

crypttokenplugin_LTLIBRARIES=ddvcard.la
noinst_HEADERS=ddvcard_p.h

ddvcard_la_SOURCES=dummy.c

ddvcard_la_LDFLAGS=-module -no-undefined -avoid-version
ddvcard_la_LIBADD=libct_ddvcard.la ../../libchipcard/libchipcard.la $(mingw_ldflags) $(gwenhywfar_libs)


sources:
	for f in $(libct_ddvcard_la_SOURCES) $(extra_sources); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done

typefiles:

typedefs:
