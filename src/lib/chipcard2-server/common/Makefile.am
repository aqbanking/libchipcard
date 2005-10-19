INCLUDES=@all_includes@

DEFS=@DEFS@ -DLC_LOWLEVELDRIVER_PATH=\"@LC_LOWLEVELDRIVER_PATH@\"

noinst_LTLIBRARIES=libcommon.la

noinst_HEADERS=\
  card_p.h \
  devmonitor_p.h \
  usbmonitor_p.h \
  pcimonitor_p.h \
  pcimonitor_l.h \
  usbttymonitor_p.h


iheaderdir=${includedir}/chipcard2-server/common
iheader_HEADERS=\
  card.h \
  driverinfo.h \
  devmonitor.h \
  usbmonitor.h \
  usbttymonitor.h

libcommon_la_SOURCES=\
  card.c \
  driverinfo.c \
  devmonitor.c \
  usbmonitor.c \
  pcimonitor.c \
  usbttymonitor.c


MAINTAINERCLEANFILES=Makefile.in

