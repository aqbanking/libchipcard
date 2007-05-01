INCLUDES= \
  -I.. -I../.. \
  -I$(srcdir)/.. -I$(srcdir)/../.. \
  @all_includes@

noinst_LTLIBRARIES=libclientmgr.la

EXTRA_DIST= \
  clr_clientready.c \
  clr_notification.c \
  clr_startwait.c \
  clr_stopwait.c \
  clr_takecard.c \
  clr_releasecard.c \
  clr_execapdu.c \
  clr_getdrivervar.c \
  clr_lockreader.c \
  clr_unlockreader.c \
  clr_readercmd.c

noinst_HEADERS=\
  cl_request_l.h \
  cl_request_p.h \
  cl_client_l.h \
  cl_client_p.h \
  clientmanager_l.h \
  clientmanager_p.h

libclientmgr_la_SOURCES=\
  cl_request.c \
  cl_client.c \
  clientmanager.c


MAINTAINERCLEANFILES=Makefile.in

