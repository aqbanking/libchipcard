
DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)
AM_CPPFLAGS= $(gwenhywfar_CFLAGS) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

EXTRA_DIST=chiptanusb-app.xml chiptanusb-card.xml

noinst_LTLIBRARIES=libcards-chiptanusb.la
noinst_HEADERS=\
  chiptanusb_p.h


iheaderdir=@libchipcard_headerdir_am@/cards/chiptanusb
iheader_HEADERS=\
  chiptanusb.h

libcards_chiptanusb_la_SOURCES=\
  chiptanusb.c


appdatadir=@lc_client_xml_dir@/apps
appdata_DATA=chiptanusb-app.xml

carddatadir=@lc_client_xml_dir@/cards
carddata_DATA=chiptanusb-card.xml

sources:
	for f in $(libcards_chiptanusb_la_SOURCES) $(extra_sources); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done

typefiles:

typedefs:

