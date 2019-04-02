PREFIX=/usr/local
LIBDIR=$(PREFIX)/lib
BINDIR=$(PREFIX)/bin
DESTDIR=

MODULES = libtascar plugins apps gui
DOCMODULES = doc manual

all: $(MODULES)

alldoc: all $(DOCMODULES)

.PHONY : $(MODULES) $(DOCMODULES) coverage

$(MODULES:external_libs=) $(DOCMODULES):
	$(MAKE) -C $@

clean:
	for m in $(MODULES) $(DOCMODULES); do $(MAKE) -C $$m clean; done
	$(MAKE) -C test clean
	$(MAKE) -C manual clean
	$(MAKE) -C examples clean
	$(MAKE) -C external_libs clean
	rm -Rf build devkit/Makefile.local devkit/build

test:
	$(MAKE) -C test
	$(MAKE) -C examples

googletest:
	$(MAKE) -C external_libs googlemock

unit-tests: $(patsubst %,%-subdir-unit-tests,$(MODULES))
$(patsubst %,%-subdir-unit-tests,$(MODULES)): all googletest
	$(MAKE) -C $(@:-subdir-unit-tests=) unit-tests

coverage: googletest unit-tests
	lcov --capture --directory ./ --output-file coverage.info
	genhtml coverage.info --prefix $$PWD --show-details --demangle-cpp --output-directory $@
	x-www-browser ./coverage/index.html

install: all
	@echo "We recommend to install TASCAR using the binary packages."
	@echo "See http://install.tascar.org/ for installation instructions."
	@echo "Press Ctrl-C to stop installation, or Enter to continue."
	@read RESP
	cp -i libtascar/build/libtascar*.so $(DESTDIR)$(LIBDIR)
	cp -i plugins/build/*.so $(DESTDIR)$(LIBDIR)
	cp -i apps/build/tascar_* $(DESTDIR)$(BINDIR)
	cp -i gui/build/tascar $(DESTDIR)$(BINDIR)
	cp -i gui/build/tascar_spkcalib $(DESTDIR)$(BINDIR)

.PHONY : all clean test

