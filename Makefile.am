SUBDIRS=data doc m4 src po tutorials

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


pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libchipcard-client.pc

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


if IF_MAKE_I18N_FILES

# We directly read that file list. For old make programs, you would
# set this to an empty variable, because they are only needed by the
# developers and release technicians.
I18NFILES=$(shell cat $(srcdir)/i18nsources)
DLGFILES=$(shell cat $(srcdir)/dlgsources)
PDFILES=$(shell cat $(srcdir)/pdsources)

# Current build rule:
sources:
	rm -f $(srcdir)/i18nsources $(srcdir)/dlgsources $(srcdir)/pdsources
	$(MAKE) -C $(top_builddir)/src sources
$(srcdir)/i18nsources $(srcdir)/dlgsources $(srcdir)/pdsources:
	$(MAKE) sources

# Could also be solved this way: Read the I18NFILES variable from file
#include $(srcdir)/i18nsources
# (Needs the following build rule:)
#sources:
#	rm -f $(srcdir)/i18nsources
#	$(MAKE) -C $(top_srcdir)/src sources
#	mv $(srcdir)/i18nsources $(srcdir)/i18nsources.tmp
#	echo "I18NFILES = \\" > $(srcdir)/i18nsources
#	$(SED) 's!\(^.*$$\)!$$(top_srcdir)/\1 \\!' \
#	  $(srcdir)/i18nsources.tmp >> $(srcdir)/i18nsources
#	echo '$$(top_srcdir)/config.h' >> $(srcdir)/i18nsources
#	rm -f $(srcdir)/i18nsources.tmp
#	$(MAKE) Makefile

# The list of available languages (makes file name transformations much easier)
ALL_LINGUAS = $(basename $(notdir $(shell ls $(srcdir)/po/*.po)))
# For old make programs, the locales would directly be written here:
#ALL_LINGUAS = de

# List of po files. Only necessary in the dependency of the catalogs
# rule. For old make programs, you would simpy set this to an empty
# variable.
I18NLINGUAS = $(foreach lang,$(ALL_LINGUAS),$(srcdir)/po/$(lang).po)

# List of mo files. Only necessary for cleanup. For old make programs,
# you would simply set this to an empty variable.
I18NCATALOGS = $(foreach lang,$(ALL_LINGUAS),$(top_builddir)/po/$(lang).mo)
I18NCATALOGS += $(foreach lang,$(ALL_LINGUAS),$(top_builddir)/po/$(lang).qm)

po/$(PACKAGE).pot: i18nsources dlgsources pdsources $(I18NFILES)
	rm -f po/tmp*
	if test -n "$(I18NFILES)"; then \
	  $(XGETTEXT) -C -c -ki18n -ktr2i18n -kI18N -kI18S\
	    -kI18N_NOOP -ktranslate -kaliasLocale \
            -ktr -ktrUtf8 \
	    '--msgid-bugs-address=aqbanking-user@lists.aqbanking.de' \
	    -o po/tmppot \
	    $(I18NFILES) ; \
	fi
	for i in $(DLGFILES); do \
          po4a-gettextize -f xml -m $$i -p po/tmppot1 -o attributes=text && \
	  msghack -o po/tmppot2 --append po/tmppot po/tmppot1 && \
	  mv po/tmppot2 po/tmppot; \
	done
	for i in $(PDFILES); do \
          po4a-gettextize -f xml -m $$i -p po/tmppot1 -o wrap \
	    -o translated="<plugin><short> <plugin><descr>" \
	    -o untranslated="<plugin><version> <plugin><author>" \
	    && \
	  msghack -o po/tmppot2 --append po/tmppot po/tmppot1 && \
	  mv po/tmppot2 po/tmppot; \
	done
	mv po/tmppot po/$(PACKAGE).pot
	rm -f po/tmppot1


# This rule is not called automatically. It is necessary only for
# the translators, so translators only call this manually.
merge: po/$(PACKAGE).pot
	@catalogs=$(I18NLINGUAS); \
	for cat in $$catalogs; do \
	echo msgmerge -o $$cat.new $$cat ./po/$(PACKAGE).pot ; \
	msgmerge -o $$cat.new $$cat ./po/$(PACKAGE).pot ; \
	if test -s $$cat.new; then \
	  grep -v "\"POT-Creation" $$cat.new | grep -v '^#' > $$cat.new.2 ; \
	  grep -v "\"POT-Creation" $$cat | grep -v '^#' >> $$cat.new.1; \
	  if diff $$cat.new.1 $$cat.new.2; then \
		echo "  $$cat unchanged; leaving existing po file untouched."; \
		rm $$cat.new;  \
	  else  \
		mv $$cat.new $$cat ; \
	fi; \
	rm -f $$cat.new.1 $$cat.new.2 ;\
	fi ;\
	done

catalogs: $(I18NLINGUAS)
	@for i in $(ALL_LINGUAS); do \
	  filelingua=$(srcdir)/po/$$i.po; \
	  filecat=$(top_builddir)/po/$$i.mo; \
	  fileqm=$(top_builddir)/po/$$i.qm; \
	  echo "Creating translation catalogs for language \"$$i\""; \
	  $(MSGFMT) -c -o $${filecat} $${filelingua} || touch $${filelingua}; \
	  if test $(MSGFMT_SUPPORTS_QT) = "yes"; then \
	    $(MSGFMT) -o $${fileqm} --qt $${filelingua} || touch $${filelingua}; \
	  else \
	    echo "$(MSGFMT) does not have the option --qt; ignoring qt translations"; \
	  fi; \
	done



ilocaledir = $(datadir)/locale
install-mo-catalogs: catalogs
	@for i in $(ALL_LINGUAS); do \
	  filecat=$(top_builddir)/po/$$i.mo; \
	  fileqm=$(top_builddir)/po/$$i.qm; \
          echo "Installing translation catalog for language \"$$i\""; \
          $(mkinstalldirs) "$(DESTDIR)$(ilocaledir)/$$i/LC_MESSAGES"; \
          $(INSTALL_DATA) $${filecat} $(DESTDIR)$(ilocaledir)/$$i/LC_MESSAGES/$(PACKAGE).mo; \
        done
uninstall-mo-catalogs:
	@$(NORMAL_UNINSTALL)
	@for i in $(ALL_LINGUAS); do \
	  echo " rm -f '$(DESTDIR)$(ilocaledir)/$$i/LC_MESSAGES/$(PACKAGE).mo'"; \
	  rm -f "$(DESTDIR)$(ilocaledir)/$$i/LC_MESSAGES/$(PACKAGE).mo"; \
	done


install-data-local: install-mo-catalogs
uninstall-local: uninstall-mo-catalogs

CLEANFILES = $(I18NCATALOGS)

endif # if USE_I18N


typefiles:
	for d in $(SUBDIRS); do \
	  $(MAKE) -C $$d typefiles; \
	done

typedefs:
	for d in $(SUBDIRS); do \
	  $(MAKE) -C $$d typedefs; \
	done

