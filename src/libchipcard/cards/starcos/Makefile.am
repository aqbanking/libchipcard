
DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)
AM_CPPFLAGS= $(gwenhywfar_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

EXTRA_DIST=app-hvb.xml app-vr.xml app.xml card.xml types.xml

typefiles=types.xml


noinst_LTLIBRARIES=libcards-starcos.la
noinst_HEADERS=\
  starcos_p.h \
  starcos_keydescr_l.h \
  starcos_keydescr_p.h


iheaderdir=@libchipcard_headerdir_am@/cards
iheader_HEADERS=\
  starcos.h \
  starcos_keydescr.h

libcards_starcos_la_SOURCES=\
  starcos.c \
  starcos_keydescr.c


types: $(typefiles)
	typemaker -d CHIPCARD_API $(typefiles)



appdatadir=@lc_client_xml_dir@/apps
carddatadir=@lc_client_xml_dir@/cards

install-app-data:
	$(INSTALL) -d $(DESTDIR)$(appdatadir)
	cp $(srcdir)/app.xml $(DESTDIR)$(appdatadir)/starcoscard.xml
	cp $(srcdir)/app-hvb.xml $(DESTDIR)$(appdatadir)/starcoscard-hvb.xml
	cp $(srcdir)/app-vr.xml $(DESTDIR)$(appdatadir)/starcoscard-vr.xml

install-card-data:
	$(INSTALL) -d $(DESTDIR)$(carddatadir)
	cp $(srcdir)/card.xml $(DESTDIR)$(carddatadir)/starcoscard.xml

install-data-local: install-app-data install-card-data

