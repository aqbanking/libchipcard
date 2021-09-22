
DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)
AM_CPPFLAGS= $(gwenhywfar_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

EXTRA_DIST=zkacard-app.xml zkacard-card.xml

typefiles=types.xml


noinst_LTLIBRARIES=libcards-zkacard.la
noinst_HEADERS=\
  zkacard_p.h


iheaderdir=@libchipcard_headerdir_am@/cards
iheader_HEADERS=\
  zkacard.h

libcards_zkacard_la_SOURCES=\
  zkacard.c


appdatadir=@lc_client_xml_dir@/apps
appdata_DATA=zkacard-app.xml

carddatadir=@lc_client_xml_dir@/cards
carddata_DATA=zkacard-card.xml

sources:
	for f in $(libcards_zkacard_la_SOURCES) $(extra_sources); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done

typefiles:

typedefs:
