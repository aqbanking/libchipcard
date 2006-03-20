MAINTAINERCLEANFILES=Makefile.in

EXTRA_DIST=prepare.sh.in.in

noinst_SCRIPTS=prepare.sh

prepare.sh: prepare.sh.in
	$(SED) -e 's!@libchipcard2s_config@!$(libchipcard2s_config_am)!' prepare.sh.in >prepare.sh && chmod u+x prepare.sh

CLEANFILES = $(noinst_SCRIPTS)
