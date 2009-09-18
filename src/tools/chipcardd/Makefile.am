
INCLUDES=\
  -I$(srcdir)/../../lib/server/server \
  -I$(srcdir)/../../lib/server/ \
  -I../../lib/server/server \
  -I../../lib/server/ \
  @all_includes@

DEFS=@DEFS@ \
  -DLC_DEVICEDRIVER_PATH=\"@lc_server_driver_dir@\" \
  -DLC_DEFAULT_DHDIR=\"@lc_server_def_dh_dir@\"\
  -DLC_DEFAULT_CONFDIR=\"@lc_server_config_dir@\" \
  -DLC_DEFAULT_DATADIR=\"@lc_server_data_dir@\" \
  -DLC_DEFAULT_PIDDIR=\"@lc_server_pid_dir@\" \
  -DLC_DEFAULT_LOGDIR=\"@lc_server_log_dir@\"

EXTRA_DIST=runme.sh

# if IS_WINDOWS
if IS_WINDOWS
 mingw_ldflags=-L/mingw/lib
endif


sbin_PROGRAMS=chipcardd4
noinst_HEADERS=chipcardd_p.h

chipcardd4_SOURCES=\
  addreader.c \
  delreader.c \
  testreader.c \
  listreaders.c \
  chipcardd.c

chipcardd4_LDADD=\
  ../../lib/server/server/libchipcards.la \
  ../../lib/server/common/libcommon.la  \
  ../../lib/sharedstuff/libsharedstuff.la $(mingw_ldflags) @gwenhywfar_libs@
chipcardd4_LDFLAGS=@STRIPALL@

