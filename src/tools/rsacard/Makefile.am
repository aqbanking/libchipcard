bin_PROGRAMS=rsacard2

DEFS=-DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

noinst_HEADERS=global.h

rsacard2_SOURCES=\
  main.c \
  generatekey.c \
  activatekey.c \
  deactivatekey.c \
  exportkey.c \
  importkey.c \
  exportbankinfo.c \
  importbankinfo.c \
  initialpin.c
rsacard2_LDADD=-L../../lib/chipcard2-client -lchipcard2c

