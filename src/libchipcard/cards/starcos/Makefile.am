
DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)
AM_CPPFLAGS= $(gwenhywfar_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

EXTRA_DIST=starcos-app-hvb.xml starcos-app-vr.xml starcos-app.xml starcos-card.xml types.xml

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
appdata_DATA=starcos-app-hvb.xml starcos-app-vr.xml starcos-app.xml

carddatadir=@lc_client_xml_dir@/cards
carddata_DATA=starcos-card.xml
