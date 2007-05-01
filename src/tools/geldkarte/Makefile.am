bin_PROGRAMS=geldkarte3

INCLUDES=@all_includes@

noinst_HEADERS=global.h

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif

geldkarte3_SOURCES=\
  main.c
geldkarte3_LDADD=../../lib/client/libchipcard3c.la $(mingw_ldflags) @gwenhywfar_libs@
geldkarte3_LDFLAGS=@STRIPALL@

