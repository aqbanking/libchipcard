
DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)
AM_CPPFLAGS= $(gwenhywfar_CFLAGS) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

EXTRA_DIST=starcos-app-hvb.xml starcos-app-vr.xml starcos-app.xml starcos-card.xml

typefiles=starcos_keydescr.xml


noinst_LTLIBRARIES=libcards-starcos.la
noinst_HEADERS=\
  starcos_p.h \
  starcos_keydescr_p.h


iheaderdir=@libchipcard_headerdir_am@/cards/starcos
iheader_HEADERS=\
  starcos.h \
  starcos_keydescr.h

libcards_starcos_la_SOURCES=\
  starcos.c \
  starcos_keydescr.c



appdatadir=@lc_client_xml_dir@/apps
appdata_DATA=starcos-app-hvb.xml starcos-app-vr.xml starcos-app.xml

carddatadir=@lc_client_xml_dir@/cards
carddata_DATA=starcos-card.xml


typefiles: $(typefiles)
	$(typemaker2_exe) -I $(gwenhywfar_dir)/share/gwenhywfar/typemaker2/c -I $(srcdir) --destfolder="$(srcdir)" -D CHIPCARD_API $(typefiles)

typedefs: $(typefiles)
	$(typemaker2_exe) --defs-only -I $(gwenhywfar_dir)/share/gwenhywfar/typemaker2/c -I $(srcdir) --destfolder="$(srcdir)" -D CHIPCARD_API $(typefiles)

sources:
	for f in $(libcards_starcos_la_SOURCES) $(extra_sources); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done

