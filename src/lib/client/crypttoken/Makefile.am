
INCLUDES=@all_includes@ -I.. -I$(srcdir)/.. 

DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)

noinst_LTLIBRARIES=libctcard.la

iheaderdir=${includedir}/chipcard3/client/crypttoken
iheader_HEADERS=\
  ct_card.h

libctcard_la_SOURCES=\
  ct_card.c

