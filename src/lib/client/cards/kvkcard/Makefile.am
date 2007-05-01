
DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)

EXTRA_DIST=app.xml card.xml


INCLUDES=@all_includes@ -I../


noinst_LTLIBRARIES=libcards-kvk.la
noinst_HEADERS=\
  kvkcard_p.h


iheaderdir=${includedir}/chipcard3/client/cards
iheader_HEADERS=\
  kvkcard.h

libcards_kvk_la_SOURCES=\
  kvkcard.c


appdatadir=@lc_client_xml_dir@/apps
carddatadir=@lc_client_xml_dir@/cards

install-app-data:
	$(INSTALL) -d $(DESTDIR)$(appdatadir)
	cp app.xml $(DESTDIR)$(appdatadir)/kvkcard.xml

install-card-data:
	$(INSTALL) -d $(DESTDIR)$(carddatadir)
	cp card.xml $(DESTDIR)$(carddatadir)/kvkcard.xml

install-data-local: install-app-data install-card-data


