
EXTRA_DIST=README

noinst_PROGRAMS=\
 tutorial1a tutorial1b tutorial1c \
 tutorial2

DEFS=-DLC_DEFAULT_DATADIR=\"@libchipcard2c_config_am@\"

tutorial1a_SOURCES=tutorial1a.c
tutorial1a_LDADD=-L../src/lib/chipcard2-client -lchipcard2c

tutorial1b_SOURCES=tutorial1b.c
tutorial1b_LDADD=-L../src/lib/chipcard2-client -lchipcard2c

tutorial1c_SOURCES=tutorial1c.c
tutorial1c_LDADD=-L../src/lib/chipcard2-client -lchipcard2c

tutorial2_SOURCES=tutorial2.c
tutorial2_LDADD=-L../src/lib/chipcard2-client -lchipcard2c
