AM_CPPFLAGS=-I$(libchipcard_symlinkdir) $(gwenhywfar_includes) -I../../ -I../../client -I../../cards

crypttokenplugindir=$(gwenhywfar_plugins)/ct
crypttokenplugin_DATA=zkacard.xml

noinst_LTLIBRARIES=libct_zkacard.la
libct_zkacard_la_SOURCES=zkacard.c


crypttokenplugin_LTLIBRARIES=zkacard.la
noinst_HEADERS=\
  zkacard_p.h

zkacard_la_SOURCES=dummy.c

zkacard_la_LDFLAGS=-module -no-undefined -avoid-version
zkacard_la_LIBADD=libct_zkacard.la ../../lib/client/libchipcard.la $(mingw_ldflags) $(gwenhywfar_libs)


