INCLUDES=@all_includes@

DEFS=@DEFS@ -DLC_LOWLEVELDRIVER_PATH=\"@LC_LOWLEVELDRIVER_PATH@\"

noinst_LTLIBRARIES=libcommon.la

noinst_HEADERS=\
  card_p.h \
  devmonitor_p.h \
  pciscanner_p.h \
  pciscanner_l.h \
  pcmciascanner_p.h \
  pcmciascanner_l.h \
  usbrawscanner_p.h \
  usbrawscanner_l.h \
  usbttyscanner_p.h \
  usbttyscanner_l.h


iheaderdir=${includedir}/chipcard2-server/common
iheader_HEADERS=\
  card.h \
  driverinfo.h \
  devmonitor.h

libcommon_la_SOURCES=\
  card.c \
  driverinfo.c \
  devmonitor.c \
  pciscanner.c \
  pcmciascanner.c \
  usbrawscanner.c \
  usbttyscanner.c

libcommon_la_LIBADD=@libusb_libs@ @libsysfs_libs@

MAINTAINERCLEANFILES=Makefile.in

