bin_PROGRAMS=geldkarte

INCLUDES=@all_includes@

noinst_HEADERS=global.h

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif

geldkarte_SOURCES=\
  main.c
geldkarte_LDADD=../../lib/client/libchipcardc.la $(mingw_ldflags) @gwenhywfar_libs@
geldkarte_LDFLAGS=@STRIPALL@

