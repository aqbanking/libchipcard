bin_PROGRAMS=zkacard-tool

AM_CPPFLAGS=-I$(libchipcard_symlinkdir) $(gwenhywfar_includes)

noinst_HEADERS=global.h

zkacard_tool_SOURCES=\
  main.c \
  getpublickey.c \
  shownotepad.c \
  resetptc.c
zkacard_tool_LDADD=../../lib/client/libchipcard.la $(mingw_ldflags) @gwenhywfar_libs@
zkacard_tool_LDFLAGS=@STRIPALL@

if IS_WINDOWS
zkacard_tool_LDFLAGS+=-mconsole
endif

MAINTAINERCLEANFILES=Makefile.in

