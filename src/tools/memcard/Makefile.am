bin_PROGRAMS=memcard

INCLUDES=@all_includes@

noinst_HEADERS=global.h

memcard_SOURCES=\
  main.c
memcard_LDADD=../../lib/client/libchipcardc.la $(mingw_ldflags) @gwenhywfar_libs@
memcard_LDFLAGS=@STRIPALL@

if IS_WINDOWS
memcard_LDFLAGS+=-mconsole
endif

