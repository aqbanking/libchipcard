bin_PROGRAMS=chipcard-tool

DEFS=-DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

noinst_HEADERS=global.h

chipcard_tool_SOURCES=\
  main.c \
  checkreaders.c \
  getatr.c \
  listreaders.c
chipcard_tool_LDADD=-L../../lib/chipcard2-client -lchipcard2c

