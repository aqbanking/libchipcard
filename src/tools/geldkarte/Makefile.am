bin_PROGRAMS=geldkarte

INCLUDES=@all_includes@

noinst_HEADERS=global.h

geldkarte_SOURCES=\
  main.c
geldkarte_LDADD=../../lib/client/libchipcardc.la $(mingw_ldflags) @gwenhywfar_libs@
geldkarte_LDFLAGS=@STRIPALL@

if IS_WINDOWS
geldkarte_LDFLAGS+=-mconsole
endif
