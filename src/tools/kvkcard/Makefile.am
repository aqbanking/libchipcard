bin_PROGRAMS=kvkcard

INCLUDES=@all_includes@

EXTRA_DIST=README daemon.c read.c rdvd.c rdpd.c

noinst_HEADERS=global.h

kvkcard_SOURCES=\
  main.c
kvkcard_LDADD=../../lib/client/libchipcardc.la $(mingw_ldflags) @gwenhywfar_libs@
kvkcard_LDFLAGS=@STRIPALL@

if IS_WINDOWS
kvkcard_LDFLAGS+=-mconsole
endif


MAINTAINERCLEANFILES=Makefile.in

