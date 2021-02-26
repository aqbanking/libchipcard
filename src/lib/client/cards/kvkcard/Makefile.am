
DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)
AM_CPPFLAGS=-I$(libchipcard_symlinkdir) $(gwenhywfar_includes) -I../

EXTRA_DIST=app.xml card.xml


noinst_LTLIBRARIES=libcards-kvk.la
noinst_HEADERS=\
  kvkcard_p.h


iheaderdir=@libchipcard_headerdir_am@/cards
iheader_HEADERS=\
  kvkcard.h

libcards_kvk_la_SOURCES=\
  kvkcard.c


appdatadir=@lc_client_xml_dir@/apps
carddatadir=@lc_client_xml_dir@/cards

install-app-data:
	$(INSTALL) -d $(DESTDIR)$(appdatadir)
	cp $(srcdir)/app.xml $(DESTDIR)$(appdatadir)/kvkcard.xml

install-card-data:
	$(INSTALL) -d $(DESTDIR)$(carddatadir)
	cp $(srcdir)/card.xml $(DESTDIR)$(carddatadir)/kvkcard.xml

install-data-local: install-app-data install-card-data


