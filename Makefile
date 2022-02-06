PREFIX=/usr/local
LIBDIR=$(PREFIX)/lib
BINDIR=$(PREFIX)/bin
INCDIR=$(PREFIX)/include
DESTDIR=

MODULES = libtascar apps plugins gui
DOCMODULES = doc manual

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CMD_INSTALL=install
	LIB_EXT=so
	CMD_LD=ldconfig -n $(DESTDIR)$(LIBDIR)
endif
ifeq ($(UNAME_S),Darwin)
	CMD_INSTALL=ginstall
	LIB_EXT=dylib
	CMD_LD=
endif



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
	$(MAKE) -C packaging/deb clean
	rm -Rf build devkit/Makefile.local devkit/build

# test can not run in multi-threaded mode!
test: apps plugins
	$(MAKE) -j 1 -C test
	$(MAKE) -C examples

testjack: apps plugins
	$(MAKE) -j 1 -C test jack

googletest:
	$(MAKE) -C external_libs googlemock

unit-tests: $(patsubst %,%-subdir-unit-tests,$(MODULES))
$(patsubst %,%-subdir-unit-tests,$(MODULES)): libtascar googletest
	$(MAKE) -C $(@:-subdir-unit-tests=) unit-tests

coverage: googletest unit-tests test testjack
	lcov --capture --directory ./ --output-file coverage.info
	genhtml coverage.info --prefix $$PWD --show-details --demangle-cpp --output-directory $@
	x-www-browser ./coverage/index.html

install: all
	$(CMD_INSTALL) -D libtascar/build/libtascar*.$(LIB_EXT) -t $(DESTDIR)$(LIBDIR)
	$(CMD_INSTALL) -D libtascar/include/*.h -t $(DESTDIR)$(INCDIR)/tascar
	$(CMD_INSTALL) -D libtascar/build/*.h -t $(DESTDIR)$(INCDIR)/tascar
	$(CMD_INSTALL) -D plugins/build/*.$(LIB_EXT) -t $(DESTDIR)$(LIBDIR)
	$(CMD_INSTALL) -D apps/build/tascar_* -t $(DESTDIR)$(BINDIR)
	$(CMD_INSTALL) -D gui/build/tascar -t $(DESTDIR)$(BINDIR)
	$(CMD_INSTALL) -D gui/build/tascar_spkcalib -t $(DESTDIR)$(BINDIR)
	$(CMD_LD)

.PHONY : all clean test docexamples releasetag checkmodified checkversiontagged

docexamples:
	$(MAKE) -C manual/examples

pack: $(MODULES) $(DOCMODULES) docexamples unit-tests test
	$(MAKE) -C packaging/deb

releasepack: checkversiontagged checkmodified $(MODULES) $(DOCMODULES) docexamples unit-tests test
	$(MAKE) -C packaging/deb

fastpack: $(MODULES) $(DOCMODULES)
	$(MAKE) -C packaging/deb

include config.mk

checkmodified:
	test -z "`git status --porcelain -uno`"

checkversiontagged:
	test "`git log -1 --abbrev=7 --pretty='format:%h'`" = "`git log -1 --abbrev=7 --pretty='format:%h' release_$(VERSION)`"

releasetag: checkmodified
	git tag -a release_$(VERSION)

allwithcov: googletest
	$(MAKE) LDCOVFLAGS="-fprofile-arcs" GCCCOVFLAGS="-fprofile-arcs -ftest-coverage" COVLIBS="-lgcov" $(MODULES)

cleancov:
	find . -name "*.gcda" -exec rm -f \{\} \;
	rm -Rf coverage
	rm -f coverage.info
