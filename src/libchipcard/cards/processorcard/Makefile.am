
DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)
AM_CPPFLAGS= $(gwenhywfar_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

EXTRA_DIST=processorcard-app.xml processorcard-card.xml


noinst_LTLIBRARIES=libcards-processorcard.la
noinst_HEADERS=\
  processorcard_p.h


iheaderdir=@libchipcard_headerdir_am@/cards
iheader_HEADERS=\
  processorcard.h

libcards_processorcard_la_SOURCES=\
  processorcard.c

appdatadir=@lc_client_xml_dir@/apps
appdata_DATA=processorcard-app.xml

carddatadir=@lc_client_xml_dir@/cards
carddata_DATA=processorcard-card.xml

