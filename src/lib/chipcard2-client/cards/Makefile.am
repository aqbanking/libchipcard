DEFS=@DEFS@ \
  -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

INCLUDES=@all_includes@ -I../ -I../client -I$(srcdir)/../ -I$(srcdir)/../client

typefiles=starcos.xml geldkarte.xml


noinst_LTLIBRARIES=libcards.la
noinst_HEADERS=\
  ddvcard_p.h \
  geldkarte_p.h \
  geldkarte_blog_l.h \
  geldkarte_blog_p.h \
  geldkarte_llog_l.h \
  geldkarte_llog_p.h \
  geldkarte_values_l.h \
  geldkarte_values_p.h \
  kvkcard_p.h \
  memorycard_p.h \
  processorcard_p.h \
  starcos_p.h \
  starcos_keydescr_l.h \
  starcos_keydescr_p.h
  

iheaderdir=${includedir}/chipcard2-client/cards
iheader_HEADERS=\
  ddvcard.h \
  geldkarte.h \
  geldkarte_blog.h \
  geldkarte_llog.h \
  geldkarte_values.h \
  kvkcard.h \
  memorycard.h \
  processorcard.h \
  starcos.h \
  starcos_keydescr.h

libcards_la_SOURCES=\
  ddvcard.c \
  geldkarte.c \
  geldkarte_blog.c \
  geldkarte_llog.c \
  geldkarte_values.c \
  kvkcard.c \
  memorycard.c \
  processorcard.c \
  starcos.c \
  starcos_keydescr.c


types: $(typefiles)
	typemaker -d CHIPCARD_API $(typefiles)

MAINTAINERCLEANFILES=Makefile.in

