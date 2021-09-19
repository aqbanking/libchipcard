bin_PROGRAMS=chipcard-tool

AM_CPPFLAGS= $(gwenhywfar_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

noinst_HEADERS=global.h

chipcard_tool_SOURCES=\
  main.c \
  getatr.c
chipcard_tool_LDADD=../../libchipcard/libchipcard.la $(mingw_ldflags) @gwenhywfar_libs@
chipcard_tool_LDFLAGS=@STRIPALL@

if IS_WINDOWS
chipcard_tool_LDFLAGS+=-mconsole
endif

MAINTAINERCLEANFILES=Makefile.in

