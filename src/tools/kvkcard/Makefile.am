bin_PROGRAMS=kvkcard

AM_CPPFLAGS= $(gwenhywfar_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

EXTRA_DIST=README read.c rdvd.c rdpd.c psvd.c

noinst_HEADERS=global.h

kvkcard_SOURCES=\
  main.c
kvkcard_LDADD=../../libchipcard/libchipcard.la $(mingw_ldflags) @gwenhywfar_libs@
kvkcard_LDFLAGS=@STRIPALL@

if IS_WINDOWS
kvkcard_LDFLAGS+=-mconsole
endif


MAINTAINERCLEANFILES=Makefile.in

