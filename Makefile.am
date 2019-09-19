SUBDIRS=bin data etc doc m4 src tutorials

EXTRA_DIST= \
 AUTHORS \
 COPYING \
 ChangeLog \
 Doxyfile.in \
 INSTALL \
 NEWS \
 README \
 chipcard.m4 \
 libchipcard.spec.in libchipcard.spec Makefile.cvs

bin_SCRIPTS = chipcard-config
M4dir = $(datadir)/aclocal
M4_DATA = chipcard.m4 
ACLOCAL_AMFLAGS = -I m4

CLEANFILES = $(bin_SCRIPTS)

chipcard-config: chipcard-config.in
	echo "#! /bin/sh" >chipcard-config && \
	echo 'prefix="@prefix@"' >>chipcard-config && \
	cat chipcard-config.in >>chipcard-config && \
	chmod a+x chipcard-config

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libchipcard-client.pc libchipcard-server.pc

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
	 cp -r src/libchipcard2.dll distribution/win32-bin/ && \
	 strip distribution/win32-bin/*.dll && \
	 cd distribution && tar pczvf @PACKAGE@-@VERSION@-win32-bin.tgz win32-bin && \
	 rm -Rf win32-bin

listdoc.h:
	$(gwenhywfar_bindir)/mklistdoc -v `find chipcard -name '*.h' ` >listdoc.h


srcdoc: Doxyfile listdoc.h
	if test -d apidoc; then rm -Rf apidoc; fi
	mkdir -p apidoc/html/libchipcard-@CHIPCARD_VERSION_RELEASE_STRING@
	doxygen

install-srcdoc: srcdoc
	test -d "$(DESTDIR)@docpath@/$(PACKAGE)" || \
	  mkdir -p "$(DESTDIR)@docpath@/$(PACKAGE)"
	for f in apidoc/html/libchipcard-@CHIPCARD_VERSION_RELEASE_STRING@/*; do \
	  cp $$f "$(DESTDIR)@docpath@/$(PACKAGE)/"; \
	done
	cp "apidoc/$(PACKAGE).tag" "$(DESTDIR)@docpath@/"
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


#	git-log --pretty="format:%ad %an%n%s%n%b" >ChangeLog

ChangeLog:
	if test -d .git; then \
	  git log --pretty="format:------------------------------------------------------------------%n%ai %an%n%s%n%b" -n 100 >ChangeLog ;\
	else \
	  touch ChangeLog; \
	fi


dist-hook:
	rm -f ChangeLog 2>/dev/null
	$(MAKE) ChangeLog

format:
	find . -name '*.[c,h,cpp]' -exec astyle \
	  --style=stroustrup \
	    -s2 \
	    --min-conditional-indent=0 \
	    --indent-labels \
	    --max-continuation-indent=80 \
	    --pad-comma \
	    --pad-header \
	    --unpad-paren \
	    --align-pointer=name \
	    --break-closing-braces \
	    --break-one-line-headers \
	    --attach-return-type \
	    --convert-tabs \
	    --max-code-length=120 \
	    --break-after-logical \
	    --preserve-date \
	    --suffix=none \{\} \;

