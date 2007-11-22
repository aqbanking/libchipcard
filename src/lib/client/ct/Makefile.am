
INCLUDES=@all_includes@ -I.. -I$(srcdir)/.. 

DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)

noinst_LTLIBRARIES=liblcct.la

iheaderdir=${includedir}/chipcard/client/ct
iheader_HEADERS=\
  ct_card.h

liblcct_la_SOURCES=\
  ct_card.c

