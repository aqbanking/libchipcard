AM_CPPFLAGS= $(gwenhywfar_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

crypttokenplugindir=$(gwenhywfar_plugins)/ct
crypttokenplugin_DATA=zkacard.xml

noinst_LTLIBRARIES=libct_zkacard.la
libct_zkacard_la_SOURCES=zkacard.c


crypttokenplugin_LTLIBRARIES=zkacard.la
noinst_HEADERS=\
  zkacard_p.h

zkacard_la_SOURCES=dummy.c

zkacard_la_LDFLAGS=-module -no-undefined -avoid-version
zkacard_la_LIBADD=libct_zkacard.la ../../libchipcard/libchipcard.la $(mingw_ldflags) $(gwenhywfar_libs)


sources:
	for f in $(libct_zkacard_la_SOURCES) $(extra_sources); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done

typefiles:

typedefs:
