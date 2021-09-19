
DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)
AM_CPPFLAGS= $(gwenhywfar_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

EXTRA_DIST=app.xml card.xml

typefiles=types.xml


noinst_LTLIBRARIES=libcards-chiptanusb.la
noinst_HEADERS=\
  chiptanusb_p.h


iheaderdir=@libchipcard_headerdir_am@/cards
iheader_HEADERS=\
  chiptanusb.h

libcards_chiptanusb_la_SOURCES=\
  chiptanusb.c


types: $(typefiles)
	typemaker -d CHIPCARD_API $(typefiles)



appdatadir=@lc_client_xml_dir@/apps
carddatadir=@lc_client_xml_dir@/cards

install-app-data:
	$(INSTALL) -d $(DESTDIR)$(appdatadir)
	cp $(srcdir)/app.xml $(DESTDIR)$(appdatadir)/chiptanusb.xml

install-card-data:
	$(INSTALL) -d $(DESTDIR)$(carddatadir)
	cp $(srcdir)/card.xml $(DESTDIR)$(carddatadir)/chiptanusb.xml

install-data-local: install-app-data install-card-data

