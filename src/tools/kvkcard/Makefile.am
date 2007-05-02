bin_PROGRAMS=kvkcard

INCLUDES=@all_includes@

noinst_HEADERS=global.h

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif

kvkcard_SOURCES=\
  main.c
kvkcard_LDADD=../../lib/client/libchipcardc.la $(mingw_ldflags) @gwenhywfar_libs@
kvkcard_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

