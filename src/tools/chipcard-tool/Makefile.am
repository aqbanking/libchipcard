bin_PROGRAMS=chipcard3-tool

INCLUDES=@all_includes@

noinst_HEADERS=global.h

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif

chipcard3_tool_SOURCES=\
  main.c \
  checkreaders.c \
  getatr.c \
  listreaders.c \
  monitor.c
chipcard3_tool_LDADD=../../lib/client/libchipcard3c.la $(mingw_ldflags) @gwenhywfar_libs@
chipcard3_tool_LDFLAGS=@STRIPALL@

MAINTAINERCLEANFILES=Makefile.in

