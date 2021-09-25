SUBDIRS=memorycard processorcard ddvcard geldkarte kvkcard egkcard starcos zkacard chiptanusb

DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)
AM_CPPFLAGS= $(gwenhywfar_CFLAGS) $(pcsc_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

typefiles=\
  hipersonaldata.xml \
  hiinsurancedata.xml

EXTRA_DIST=card.xml $(typefiles)

noinst_LTLIBRARIES=libcards.la

libcards_la_SOURCES=\
  hiinsurancedata.c \
  hipersonaldata.c

iheaderdir=@libchipcard_headerdir_am@/cards
iheader_HEADERS=\
  hiinsurancedata.h \
  hipersonaldata.h

noinst_HEADERS=\
  hiinsurancedata_p.h \
  hipersonaldata_p.h

libcards_la_LIBADD=\
  ddvcard/libcards-ddv.la \
  egkcard/libcards-egk.la \
  geldkarte/libcards-geldkarte.la \
  kvkcard/libcards-kvk.la \
  starcos/libcards-starcos.la \
  zkacard/libcards-zkacard.la \
  chiptanusb/libcards-chiptanusb.la \
  memorycard/libcards-memorycard.la \
  processorcard/libcards-processorcard.la

appdatadir=@lc_client_xml_dir@/apps
appdata_DATA=

carddatadir=@lc_client_xml_dir@/cards
carddata_DATA=card.xml


typefiles: $(typefiles)
	$(typemaker2_exe) -I $(gwenhywfar_dir)/share/gwenhywfar/typemaker2/c -I $(srcdir) --destfolder="$(srcdir)" -D CHIPCARD_API $(typefiles)

typedefs: $(typefiles)
	$(typemaker2_exe) --defs-only -I $(gwenhywfar_dir)/share/gwenhywfar/typemaker2/c -I $(srcdir) --destfolder="$(srcdir)" -D CHIPCARD_API $(typefiles)


sources:
	for f in $(libcards_la_SOURCES) $(extra_sources); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done
	for d in $(SUBDIRS); do \
	  $(MAKE) -C $$d sources; \
	done

