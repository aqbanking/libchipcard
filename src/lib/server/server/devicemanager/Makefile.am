INCLUDES= \
  -I.. -I../.. -I../../common \
  -I$(srcdir)/.. -I$(srcdir)/../.. -I$(srcdir)/../../common \
  @all_includes@

noinst_LTLIBRARIES=libdevmgr.la

noinst_HEADERS=\
  devicemanager_p.h \
  devicemanager_l.h \
  dm_card_l.h \
  dm_card_p.h \
  dm_driver_p.h \
  dm_driver_l.h \
  dm_reader_p.h \
  dm_reader_l.h \
  dm_slot_l.h \
  dm_slot_p.h

libdevmgr_la_SOURCES=\
  devicemanager.c \
  dm_card.c \
  dm_driver.c \
  dm_reader.c \
  dm_slot.c


MAINTAINERCLEANFILES=Makefile.in

