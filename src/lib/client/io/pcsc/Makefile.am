INCLUDES=@gwenhywfar_includes@

noinst_LTLIBRARIES=libio_pcsc.la

DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)

EXTRA_DIST=mywinscard.c

noinst_HEADERS=\
  cardpcsc_l.h cardpcsc_p.h \
  clientpcsc_l.h clientpcsc_p.h \
  readerpcsc_l.h readerpcsc_p.h

iheaderdir=${includedir}/chipcard/client/io/pcsc
iheader_HEADERS=\
  clientpcsc.h

libio_pcsc_la_SOURCES=\
  cardpcsc.c \
  clientpcsc.c \
  readerpcsc.c


