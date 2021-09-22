SUBDIRS=geldkarte chipcard-tool memcard kvkcard cardcommander zkacard usbtan-test

sources:
	for d in $(SUBDIRS); do \
	  $(MAKE) -C $$d sources; \
	done
