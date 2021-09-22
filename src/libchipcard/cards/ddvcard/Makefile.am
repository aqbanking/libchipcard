
DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)
AM_CPPFLAGS= $(gwenhywfar_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

EXTRA_DIST=\
  ddvcard-app.xml ddvcard-app-ddv0.xml ddvcard-app-ddv1.xml \
  ddvcard-card-ddv0.xml ddvcard-card-ddv1.xml


noinst_LTLIBRARIES=libcards-ddv.la
noinst_HEADERS=\
  ddvcard_p.h


iheaderdir=@libchipcard_headerdir_am@/cards
iheader_HEADERS=\
  ddvcard.h

libcards_ddv_la_SOURCES=\
  ddvcard.c

appdatadir=@lc_client_xml_dir@/apps
appdata_DATA=ddvcard-app.xml ddvcard-app-ddv0.xml ddvcard-app-ddv1.xml

carddatadir=@lc_client_xml_dir@/cards
carddata_DATA=ddvcard-card-ddv0.xml ddvcard-card-ddv1.xml


sources:
	for f in $(libcards_ddv_la_SOURCES) $(extra_sources); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done

typefiles:

typedefs:
