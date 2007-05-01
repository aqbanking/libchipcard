INCLUDES=@all_includes@

DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)

EXTRA_DIST=\
  d_handle.c \
  d_cardcommand.c \
  d_resetcard.c \
  d_resumecheck.c \
  d_startreader.c \
  d_stopdriver.c \
  d_stopreader.c \
  d_suspendcheck.c \
  d_ipc.c \
  d_virtual.c \
  d_work.c

lib_LTLIBRARIES=libchipcard3d.la

iheaderdir=${includedir}/chipcard3/server/driver
iheader_HEADERS=\
  driver.h \
  reader.h \
  slot.h

noinst_HEADERS=\
  driver_p.h \
  reader_p.h \
  slot_p.h


libchipcard3d_la_SOURCES=\
  driver.c \
  reader.c \
  slot.c

libchipcard3d_la_LIBADD= \
  ../common/libcommon.la \
  ../../sharedstuff/libsharedstuff.la @gwenhywfar_libs@


