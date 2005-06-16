INCLUDES=@all_includes@

DEFS=@DEFS@ -DLC_LOWLEVELDRIVER_PATH=\"@LC_LOWLEVELDRIVER_PATH@\"

noinst_LTLIBRARIES=libcommon.la

noinst_HEADERS=\
  usbmonitor_p.h \
  usbttymonitor_p.h


iheaderdir=${includedir}/chipcard2-server/common
iheader_HEADERS=\
  driverinfo.h \
  usbmonitor.h \
  usbttymonitor.h

libcommon_la_SOURCES=\
  driverinfo.c \
  usbmonitor.c \
  usbttymonitor.c


MAINTAINERCLEANFILES=Makefile.in

