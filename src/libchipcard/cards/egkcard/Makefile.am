
DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)
AM_CPPFLAGS= $(gwenhywfar_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

EXTRA_DIST=egkcard-app.xml egkcard-card.xml


noinst_LTLIBRARIES=libcards-egk.la
noinst_HEADERS=\
  egkcard_p.h


iheaderdir=@libchipcard_headerdir_am@/cards
iheader_HEADERS=\
  egkcard.h

libcards_egk_la_SOURCES=\
  egkcard.c

appdatadir=@lc_client_xml_dir@/apps
appdata_DATA=egkcard-app.xml

carddatadir=@lc_client_xml_dir@/cards
carddata_DATA=egkcard-card.xml


sources:
	for f in $(libcards_egk_la_SOURCES) $(extra_sources); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done

