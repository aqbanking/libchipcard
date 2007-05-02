INCLUDES=@all_includes@

EXTRA_DIST=kvks.xml.in kvks.xml

servicedir=@lc_server_service_dir@
service_PROGRAMS=kvks
service_DATA=kvks.xml


noinst_HEADERS=\
  kvks_p.h \
  kvkscard_p.h \
  kvkscard.h

kvks_SOURCES=\
  kvks.c \
  kvkscard.c

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif

kvks_LDADD=\
 ../../lib/client/libchipcardc.la $(mingw_ldflags) @gwenhywfar_libs@
kvks_LDFLAGS=@STRIPALL@


