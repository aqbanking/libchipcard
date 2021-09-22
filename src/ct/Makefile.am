SUBDIRS=ddvcard starcoscard zkacard chiptanusb


sources:
	for d in $(SUBDIRS); do \
	  $(MAKE) -C $$d sources; \
	done


typefiles:
	for d in $(SUBDIRS); do \
	  $(MAKE) -C $$d typefiles; \
	done

typedefs:
	for d in $(SUBDIRS); do \
	  $(MAKE) -C $$d typedefs; \
	done

