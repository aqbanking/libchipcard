bin_PROGRAMS=memcard

INCLUDES=@all_includes@

noinst_HEADERS=global.h

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif

memcard_SOURCES=\
  main.c
memcard_LDADD=../../lib/client/libchipcardc.la $(mingw_ldflags) @gwenhywfar_libs@
memcard_LDFLAGS=@STRIPALL@

