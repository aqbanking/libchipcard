bin_PROGRAMS=memcard3

INCLUDES=@all_includes@

noinst_HEADERS=global.h

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif

memcard3_SOURCES=\
  main.c
memcard3_LDADD=../../lib/client/libchipcard3c.la $(mingw_ldflags) @gwenhywfar_libs@
memcard3_LDFLAGS=@STRIPALL@

