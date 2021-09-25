noinst_LTLIBRARIES=libbase.la

EXTRA_DIST=\
  README.apps README.cards


DEFS += \
  -DBUILDING_CHIPCARD \
  -DLC_CLIENT_XML_DIR=\"@lc_data_searchdir@\" \
  -DLC_CLIENT_LOCALE_DIR=\"@lc_locale_searchdir@\"

AM_CFLAGS=$(visibility_cflags)
AM_CPPFLAGS= $(gwenhywfar_CFLAGS) $(pcsc_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

typefiles=pininfo.xml


iheaderdir=@libchipcard_headerdir_am@
iheader_HEADERS=\
 client.h \
 card.h card_imp.h \
 tlv.h \
 pininfo.h

noinst_HEADERS=\
 client_l.h client_p.h client_cmd.h client_xml.h \
 card_l.h card_p.h \
 tlv_p.h \
 driverinfo.h driverinfo_p.h \
 msgengine.h msgengine_p.h \
 pininfo_p.h

libbase_la_SOURCES=\
 client.c \
 client_cmd.c \
 client_io.c \
 client_xml.c \
 card.c \
 card_data.c \
 card_files.c \
 card_iso.c \
 tlv.c \
 driverinfo.c \
 misc.c \
 msgengine.c \
 pininfo.c

appdatadir=@lc_client_xml_dir@/apps
carddatadir=@lc_client_xml_dir@/cards

install-app-data:
	$(INSTALL) -d $(DESTDIR)$(appdatadir)
	cp $(srcdir)/README.apps $(DESTDIR)$(appdatadir)/README

install-card-data:
	$(INSTALL) -d $(DESTDIR)$(carddatadir)
	cp $(srcdir)/README.cards $(DESTDIR)$(carddatadir)/README

install-data-local: install-app-data install-card-data

typefiles: $(typefiles)
	$(typemaker2_exe) -I $(gwenhywfar_dir)/share/gwenhywfar/typemaker2/c -I $(srcdir) --destfolder="$(srcdir)" -D CHIPCARD_API $(typefiles)

typedefs: $(typefiles)
	$(typemaker2_exe) --defs-only -I $(gwenhywfar_dir)/share/gwenhywfar/typemaker2/c -I $(srcdir) --destfolder="$(srcdir)" -D CHIPCARD_API $(typefiles)


sources:
	for f in $(libbase_la_SOURCES) $(extra_sources); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done
	for d in $(SUBDIRS); do \
	  $(MAKE) -C $$d sources; \
	done


