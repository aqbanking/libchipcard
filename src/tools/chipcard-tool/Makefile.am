bin_PROGRAMS=chipcard-tool

INCLUDES=@all_includes@

noinst_HEADERS=global.h

chipcard_tool_SOURCES=\
  main.c \
  checkreaders.c \
  getatr.c \
  listreaders.c \
  monitor.c
chipcard_tool_LDADD=../../lib/client/libchipcardc.la $(mingw_ldflags) @gwenhywfar_libs@
chipcard_tool_LDFLAGS=@STRIPALL@

if IS_WINDOWS
chipcard_tool_LDFLAGS+=-mconsole
endif

MAINTAINERCLEANFILES=Makefile.in

