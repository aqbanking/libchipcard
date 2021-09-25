
DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)
AM_CPPFLAGS= $(gwenhywfar_CFLAGS) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

EXTRA_DIST=kvkcard-app.xml kvkcard-card.xml


noinst_LTLIBRARIES=libcards-kvk.la
noinst_HEADERS=\
  kvkcard_p.h


iheaderdir=@libchipcard_headerdir_am@/cards/kvkcard
iheader_HEADERS=\
  kvkcard.h

libcards_kvk_la_SOURCES=\
  kvkcard.c


appdatadir=@lc_client_xml_dir@/apps
appdata_DATA=kvkcard-app.xml

carddatadir=@lc_client_xml_dir@/cards
carddata_DATA=kvkcard-card.xml


sources:
	for f in $(libcards_kvk_la_SOURCES) $(extra_sources); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done

typefiles:

typedefs:
