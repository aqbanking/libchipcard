INCLUDES=-I$(libchipcard_symlinkdir) $(gwenhywfar_includes) -I../../ -I../../client -I../../cards

crypttokenplugindir=$(gwenhywfar_plugins)/ct
crypttokenplugin_DATA=chiptanusb.xml

noinst_LTLIBRARIES=libct_chiptanusb.la
libct_chiptanusb_la_SOURCES=\
  chiptanusb.c


crypttokenplugin_LTLIBRARIES=chiptanusb.la
noinst_HEADERS=\
  chiptanusb_p.h

chiptanusb_la_SOURCES=dummy.c

chiptanusb_la_LDFLAGS=-module -no-undefined -avoid-version
chiptanusb_la_LIBADD=libct_chiptanusb.la ../../lib/client/libchipcard.la $(mingw_ldflags) $(gwenhywfar_libs)


