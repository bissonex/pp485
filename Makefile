BUILDSUBDIRS = driver examples

all::
	for i in $(BUILDSUBDIRS) ; do (cd $$i; make) || exit 1 ; done

	@echo
	@echo "PMC485 card driver and example programs built."
	@echo

clean::
	for i in $(BUILDSUBDIRS) ; do (cd $$i; make clean) || exit 1 ; done

install::
	for i in $(BUILDSUBDIRS) ; do (cd $$i; make install) || exit 1 ; done

