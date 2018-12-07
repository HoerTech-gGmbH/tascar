MODULES = libtascar plugins apps gui
DOCMODULES = doc

all: $(MODULES)

alldoc: all $(DOCMODULES)

.PHONY : $(MODULES) $(DOCMODULES) coverage

$(MODULES:external_libs=) $(DOCMODULES):
	$(MAKE) -C $@

clean:
	for m in $(MODULES) $(DOCMODULES); do $(MAKE) -C $$m clean; done
	$(MAKE) -C test clean
	$(MAKE) -C examples clean
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

googletest:
	$(MAKE) -C external_libs googlemock

unit-tests: $(patsubst %,%-subdir-unit-tests,$(MODULES))
$(patsubst %,%-subdir-unit-tests,$(MODULES)): all googletest
	$(MAKE) -C $(@:-subdir-unit-tests=) unit-tests

coverage: googletest unit-tests
	lcov --capture --directory ./ --output-file coverage.info
	genhtml coverage.info --prefix $$PWD --show-details --demangle-cpp --output-directory $@
	x-www-browser ./coverage/index.html

.PHONY : all clean test
