INCLUDES=@all_includes@

servicedir=@lc_server_service_dir@
service_PROGRAMS=cardfs

if IS_WINDOWS
  CLIENT_LIB=-lchipcardc32_$(LIBCHIPCARD_SO_EFFECTIVE)
else
  CLIENT_LIB=-lchipcardc
endif

noinst_HEADERS=\
  cardfs_p.h

cardfs_SOURCES=\
  cardfs.c

cardfs_LDADD=\
 -L../../lib/chipcard-client $(CLIENT_LIB) @gwenhywfar_libs@
cardfs_LDFLAGS=@STRIPALL@

