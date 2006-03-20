MAINTAINERCLEANFILES=Makefile.in

SUBDIRS=bin data doc m4 src tutorials

EXTRA_DIST= \
 AUTHORS \
 COPYING \
 ChangeLog \
 Doxyfile.in \
 INSTALL \
 NEWS \
 README \
 chipcard2-server.m4 \
 chipcard2-client.m4 \
 libchipcard2.spec.in libchipcard2.spec

bin_SCRIPTS = chipcard2-server-config chipcard2-client-config
M4dir = $(datadir)/aclocal
M4_DATA = chipcard2-server.m4 chipcard2-client.m4
ACLOCAL_AMFLAGS = -I m4

CLEANFILES = $(bin_SCRIPTS)

chipcard2-server-config: chipcard2-server-config.in
	echo "#! /bin/sh" >chipcard2-server-config.tmp && \
	echo 'prefix="@prefix@"' >>chipcard2-server-config.tmp && \
	cat chipcard2-server-config.in >>chipcard2-server-config.tmp && \
	$(SED) -e 's!@libchipcard2s_config@!$(libchipcard2s_config_am)!' \
	        chipcard2-server-config.tmp >chipcard2-server-config && \
	chmod a+x chipcard2-server-config
	rm -f chipcard2-server-config.tmp

chipcard2-client-config: chipcard2-client-config.in
	echo "#! /bin/sh" >chipcard2-client-config.tmp && \
	echo 'prefix="@prefix@"' >>chipcard2-client-config.tmp && \
	cat chipcard2-client-config.in >>chipcard2-client-config.tmp && \
	$(SED) -e 's!@libchipcard2s_config@!$(libchipcard2s_config_am)!' \
	        chipcard2-client-config.tmp >chipcard2-client-config && \
	chmod a+x chipcard2-client-config
	rm -f chipcard2-client-config.tmp

rpm:    $(PACKAGE).spec dist
	rpmbuild="rpm" && \
	if [ `rpm --version | awk '{ print $$3 }'` > /dev/null ]; then rpmbuild="rpmbuild"; fi && \
	$$rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz


win32:
	mkdir -p distribution
	make prefix="`pwd`/distribution/win32-tmp" install && \
	 cd distribution && mkdir -p win32-devel && \
	 cp -r win32-tmp/bin win32-devel/ && \
	 cp -r win32-tmp/include win32-devel/ && \
	 cp -r win32-tmp/share win32-devel && \
	 tar pczvf @PACKAGE@-@VERSION@-win32-devel.tgz win32-devel && \
	 rm -Rf win32-devel && \
	 rm -Rf win32-tmp
	mkdir -p distribution/win32-bin && \
	 cp -r src/libchipcard32.dll distribution/win32-bin/ && \
	 strip distribution/win32-bin/*.dll && \
	 cd distribution && tar pczvf @PACKAGE@-@VERSION@-win32-bin.tgz win32-bin && \
	 rm -Rf win32-bin


srcdoc: Doxyfile
	doxygen

$(PACKAGE).tag: Doxyfile listdoc.h
	doxygen

install-srcdoc: $(PACKAGE).tag
	test -d "$(DESTDIR)@docpath@/$(PACKAGE)" || \
	  mkdir -p "$(DESTDIR)@docpath@/$(PACKAGE)"
	cp apidoc/* "$(DESTDIR)@docpath@/$(PACKAGE)"
	cp "$(PACKAGE).tag" "$(DESTDIR)@docpath@/"
	if test -x "$(DESTDIR)@docpath@/$(PACKAGE)/installdox"; then \
	  cd "$(DESTDIR)@docpath@/$(PACKAGE)" && \
	   ID_OPT="-q" ; \
	   for ff in ${DOXYGEN_TAGFILES}; do \
	    ID_OPT="$$ID_OPT -l `basename $$ff`@../`basename $$ff | $(SED) -e s/.tag//`";\
	   done ;\
	   ( cd "$(DESTDIR)@docpath@/$(PACKAGE)" && \
	      ./installdox -q $$ID_OPT \
	   ); \
	fi


install-data-local:
	$(mkinstalldirs) "$(DESTDIR)/@LC_DEFAULT_PIDDIR@"
	$(mkinstalldirs) "$(DESTDIR)/@LC_DEFAULT_LOGDIR@"
	$(mkinstalldirs) "$(DESTDIR)/@LC_DEFAULT_NEWCERTDIR@"
	$(mkinstalldirs) "$(DESTDIR)/@LC_LOWLEVELDRIVER_PATH@"


