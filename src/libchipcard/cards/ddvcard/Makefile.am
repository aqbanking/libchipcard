
DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)
AM_CPPFLAGS= $(gwenhywfar_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

EXTRA_DIST=\
  app.xml app-ddv0.xml app-ddv1.xml \
  card-ddv0.xml card-ddv1.xml


noinst_LTLIBRARIES=libcards-ddv.la
noinst_HEADERS=\
  ddvcard_p.h


iheaderdir=@libchipcard_headerdir_am@/cards
iheader_HEADERS=\
  ddvcard.h

libcards_ddv_la_SOURCES=\
  ddvcard.c

appdatadir=@lc_client_xml_dir@/apps
carddatadir=@lc_client_xml_dir@/cards

install-app-data:
	$(INSTALL) -d $(DESTDIR)$(appdatadir)
	cp $(srcdir)/app.xml $(DESTDIR)$(appdatadir)/ddv.xml
	cp $(srcdir)/app-ddv0.xml $(DESTDIR)$(appdatadir)/ddv0.xml
	cp $(srcdir)/app-ddv1.xml $(DESTDIR)$(appdatadir)/ddv1.xml

install-card-data:
	$(INSTALL) -d $(DESTDIR)$(carddatadir)
	cp $(srcdir)/card-ddv0.xml $(DESTDIR)$(carddatadir)/ddv0.xml
	cp $(srcdir)/card-ddv1.xml $(DESTDIR)$(carddatadir)/ddv1.xml

install-data-local: install-app-data install-card-data

