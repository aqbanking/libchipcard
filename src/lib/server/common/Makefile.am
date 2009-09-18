INCLUDES=-I.. -I$(srcdir)/.. @all_includes@ @hal_includes@

noinst_LTLIBRARIES=libcommon.la

noinst_HEADERS=\
  card_p.h \
  devmonitor_p.h \
  halscanner_l.h \
  halscanner_p.h \
  pciscanner_p.h \
  pciscanner_l.h \
  reader_p.h


iheaderdir=${includedir}/chipcard/server/common
iheader_HEADERS=\
  card.h \
  devmonitor.h \
  reader.h

libcommon_la_SOURCES=\
  card.c \
  devmonitor.c \
  halscanner.c \
  pciscanner.c \
  reader.c

libcommon_la_LIBADD=@libusb_libs@ @hal_libs@

MAINTAINERCLEANFILES=Makefile.in

