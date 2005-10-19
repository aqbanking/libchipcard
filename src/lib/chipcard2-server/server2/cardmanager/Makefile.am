INCLUDES=-I.. -I../.. @all_includes@

typefiles=lockreq.xml
EXTRA_DIST=$(typefiles)

noinst_LTLIBRARIES=libcardmgr.la

noinst_HEADERS=\
  cardmanager_p.h \
  cardmanager_l.h \
  cm_card_l.h \
  cm_card_p.h \
  lockrequest_l.h \
  lockrequest_p.h

libcardmgr_la_SOURCES=\
  cardmanager.c \
  cm_card.c \
  lockrequest.c


types: $(typefiles)
	typemaker $(typefiles)


MAINTAINERCLEANFILES=Makefile.in

