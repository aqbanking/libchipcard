
DEFS += \
  -DBUILDING_CHIPCARD

AM_CFLAGS=$(visibility_cflags)
AM_CPPFLAGS= $(gwenhywfar_includes) \
  -I$(top_srcdir) -I$(top_builddir) \
  -I$(top_srcdir)/src -I$(top_builddir)/src

EXTRA_DIST=geldkarte-app.xml geldkarte-card.xml

typefiles=\
  geldkarte_blog.xml \
  geldkarte_llog.xml \
  geldkarte_values.xml


noinst_LTLIBRARIES=libcards-geldkarte.la
noinst_HEADERS=\
  geldkarte_p.h \
  geldkarte_blog_p.h \
  geldkarte_llog_p.h \
  geldkarte_values_p.h


iheaderdir=@libchipcard_headerdir_am@/cards
iheader_HEADERS=\
  geldkarte.h \
  geldkarte_blog.h \
  geldkarte_llog.h \
  geldkarte_values.h

libcards_geldkarte_la_SOURCES=\
  geldkarte.c \
  geldkarte_blog.c \
  geldkarte_llog.c \
  geldkarte_values.c

appdatadir=@lc_client_xml_dir@/apps
appdata_DATA=geldkarte-app.xml

carddatadir=@lc_client_xml_dir@/cards
carddata_DATA=geldkarte-card.xml


typefiles: $(typefiles)
	$(typemaker2_exe) -I $(gwenhywfar_dir)/share/gwenhywfar/typemaker2/c -I $(srcdir) --destfolder="$(srcdir)" -D CHIPCARD_API $(typefiles)

typedefs: $(typefiles)
	$(typemaker2_exe) --defs-only -I $(gwenhywfar_dir)/share/gwenhywfar/typemaker2/c -I $(srcdir) --destfolder="$(srcdir)" -D CHIPCARD_API $(typefiles)


sources:
	for f in $(libcards_geldkarte_la_SOURCES) $(extra_sources); do \
	  echo $(subdir)/$$f >>$(top_srcdir)/i18nsources; \
	done

