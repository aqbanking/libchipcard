INCLUDES=@all_includes@

noinst_LTLIBRARIES=libcommands.la

AM_CFLAGS=\
  -DLC_DEVICEDRIVER_PATH=\"@LC_DEVICEDRIVER_PATH@\" \
  -DLC_DEFAULT_DHFILE=\"@LC_DEFAULT_DHFILE@\"\
  -DLC_DEFAULT_DATADIR=\"@libchipcard2s_config_am@\"

noinst_HEADERS=\
  cardcontext_p.h \
  cardcontext_l.h \
  cardmgr_l.h \
  cardmgr_p.h \
  msgengine_l.h \
  msgengine_p.h

libcommands_la_SOURCES=\
  cardcontext.c \
  cardmgr.c \
  msgengine.c

