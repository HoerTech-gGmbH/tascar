all:
	$(MAKE) -C libtascar
	$(MAKE) -C plugins
	$(MAKE) -C apps
	$(MAKE) -C gui
	$(MAKE) -C doc

clean:
	$(MAKE) -C libtascar clean
	$(MAKE) -C plugins clean
	$(MAKE) -C apps clean
	$(MAKE) -C gui clean
	$(MAKE) -C test clean
	$(MAKE) -C examples clean
	$(MAKE) -C doc clean
	rm -Rf build devkit/Makefile.local devkit/build

test:
	$(MAKE) -C test
	$(MAKE) -C examples

#testdevkit: devkit/Makefile.local
#	(cd devkit && LD_LIBRARY_PATH=../libtascar/build $(MAKE) -f Makefile.local all test)
#
#%/Makefile.local: %/Makefile
#	cat $< | sed -e 's/\/usr\/share\/tascar/../1' -e 's/tascar_validate/..\/apps\/build\/tascar_validate/g' -e 's/tascar_cli/..\/apps\/build\/tascar_cli/g' > $@
#

.PHONY : all clean test
