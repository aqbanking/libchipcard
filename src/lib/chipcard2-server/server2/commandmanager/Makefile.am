INCLUDES= \
  -I.. -I../.. \
  -I$(srcdir)/.. -I$(srcdir)/../.. \
  -I$(top_srcdir)/src/lib/sharedstuff @all_includes@

typefiles=cmdrequest.xml
EXTRA_DIST=$(typefiles)

noinst_LTLIBRARIES=libcommandmgr.la

noinst_HEADERS=\
  commandmanager_l.h \
  commandmanager_p.h \
  cmd_card_l.h \
  cmd_card_p.h \
  cmdrequest_l.h \
  cmdrequest_p.h

libcommandmgr_la_SOURCES=\
  commandmanager.c \
  cmd_card.c \
  cmdrequest.c

types: $(typefiles)
	typemaker $(typefiles)


MAINTAINERCLEANFILES=Makefile.in

