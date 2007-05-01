INCLUDES=@all_includes@

servicedir=@lc_server_service_dir@
service_PROGRAMS=cardfs

if IS_WINDOWS
  CLIENT_LIB=-lchipcard3c32_$(LIBCHIPCARD_SO_EFFECTIVE)
else
  CLIENT_LIB=-lchipcard3c
endif

noinst_HEADERS=\
  cardfs_p.h

cardfs_SOURCES=\
  cardfs.c

cardfs_LDADD=\
 -L../../lib/chipcard3-client $(CLIENT_LIB) @gwenhywfar_libs@
cardfs_LDFLAGS=@STRIPALL@

