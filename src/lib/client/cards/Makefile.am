SUBDIRS=memorycard processorcard ddvcard geldkarte kvkcard egkcard starcos
DIST_SUBDIRS=memorycard processorcard ddvcard geldkarte kvkcard egkcard starcos

DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)

EXTRA_DIST=card.xml

INCLUDES=@all_includes@ -I../ -I../client -I$(srcdir)/../ -I$(srcdir)/../client

noinst_LTLIBRARIES=libcards.la

libcards_la_SOURCES=dummy.c

libcards_la_LIBADD=\
  ddvcard/libcards-ddv.la \
  egkcard/libcards-egk.la \
  geldkarte/libcards-geldkarte.la \
  kvkcard/libcards-kvk.la \
  memorycard/libcards-memorycard.la \
  processorcard/libcards-processorcard.la

appdatadir=@lc_client_xml_dir@/apps
carddatadir=@lc_client_xml_dir@/cards


install-card-data:
	$(INSTALL) -d $(DESTDIR)$(carddatadir)
	cp card.xml $(DESTDIR)$(carddatadir)/card.xml

install-data-local: install-card-data

