AM_CFLAGS=$(visibility_cflags)
AM_CPPFLAGS= $(gwenhywfar_CFLAGS) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

crypttokenplugindir=$(gwenhywfar_plugins)/ct
crypttokenplugin_DATA=chiptanusb.xml

noinst_LTLIBRARIES=libct_chiptanusb.la
libct_chiptanusb_la_SOURCES=\
  chiptanusb.c


crypttokenplugin_LTLIBRARIES=chiptanusb.la
noinst_HEADERS= \
  chiptanusb.h

chiptanusb_la_SOURCES=dummy.c

chiptanusb_la_LDFLAGS=-module -no-undefined -avoid-version
chiptanusb_la_LIBADD=libct_chiptanusb.la ../../libchipcard/libchipcard.la $(mingw_ldflags) $(gwenhywfar_LIBS)


sources:
	for f in $(libct_chiptanusb_la_SOURCES) $(extra_sources); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done

typefiles:

typedefs:
