INCLUDES=@all_includes@

noinst_LTLIBRARIES=libapps.la

DEFS=@DEFS@ \
  -DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

noinst_HEADERS=\
  cardcontext_p.h \
  cardcontext_l.h \
  cardmgr_l.h \
  cardmgr_p.h

libapps_la_SOURCES=\
  cardcontext.c \
  cardmgr.c

