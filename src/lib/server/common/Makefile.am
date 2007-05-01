INCLUDES=-I.. @all_includes@

DEFS=@DEFS@ -DLC_LOWLEVELDRIVER_PATH=\"@lc_server_lowlevel_driver_dir@\"

noinst_LTLIBRARIES=libcommon.la

noinst_HEADERS=\
  card_p.h \
  devmonitor_p.h \
  driverinfo_p.h \
  pciscanner_p.h \
  pciscanner_l.h \
  pcmciascanner_p.h \
  pcmciascanner_l.h \
  reader_p.h \
  usbrawscanner_p.h \
  usbrawscanner_l.h \
  usbttyscanner_p.h \
  usbttyscanner_l.h


iheaderdir=${includedir}/chipcard3/server/common
iheader_HEADERS=\
  card.h \
  driverinfo.h \
  devmonitor.h \
  reader.h

libcommon_la_SOURCES=\
  card.c \
  driverinfo.c \
  devmonitor.c \
  pciscanner.c \
  pcmciascanner.c \
  reader.c \
  usbrawscanner.c \
  usbttyscanner.c

libcommon_la_LIBADD=@libusb_libs@ @libsysfs_libs@

MAINTAINERCLEANFILES=Makefile.in

