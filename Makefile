PREFIX=/usr/local
LIBDIR=$(PREFIX)/lib
BINDIR=$(PREFIX)/bin
INCDIR=$(PREFIX)/include
DESTDIR=

MODULES = libtascar apps plugins gui
DOCMODULES = doc manual

all: $(MODULES)

apps plugins gui: libtascar

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

# test can not run in multi-threaded mode!
test: apps plugins
	$(MAKE) -j 1 -C test
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
	install -D libtascar/build/libtascar*.so -t $(DESTDIR)$(LIBDIR)
	install -D libtascar/src/*.h -t $(DESTDIR)$(INCDIR)/tascar
	install -D libtascar/build/*.h -t $(DESTDIR)$(INCDIR)/tascar
	install -D plugins/build/*.so -t $(DESTDIR)$(LIBDIR)
	install -D apps/build/tascar_* -t $(DESTDIR)$(BINDIR)
	install -D gui/build/tascar -t $(DESTDIR)$(BINDIR)
	install -D gui/build/tascar_spkcalib -t $(DESTDIR)$(BINDIR)
	ldconfig -n $(DESTDIR)$(LIBDIR)

.PHONY : all clean test docexamples

docexamples:
	$(MAKE) -C manual/examples


pack: $(MODULES) $(DOCMODULES) docexamples unit-tests test
	$(MAKE) -C packaging/deb
