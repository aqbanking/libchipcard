bin_PROGRAMS=kvkcard3

INCLUDES=@all_includes@

noinst_HEADERS=global.h

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif

kvkcard3_SOURCES=\
  main.c
kvkcard3_LDADD=../../lib/client/libchipcard3c.la $(mingw_ldflags) @gwenhywfar_libs@
kvkcard3_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

