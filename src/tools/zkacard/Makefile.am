bin_PROGRAMS=zkacard-tool

AM_CPPFLAGS= $(gwenhywfar_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

noinst_HEADERS=global.h

zkacard_tool_SOURCES=\
  main.c \
  getpublickey.c \
  shownotepad.c \
  resetptc.c
zkacard_tool_LDADD=../../libchipcard/libchipcard.la $(mingw_ldflags) @gwenhywfar_libs@
zkacard_tool_LDFLAGS=@STRIPALL@

if IS_WINDOWS
zkacard_tool_LDFLAGS+=-mconsole
endif

MAINTAINERCLEANFILES=Makefile.in

sources:
	for f in $(zkacard_tool_SOURCES); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done

typefiles:

typedefs:
