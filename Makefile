all:
	$(MAKE) -C libtascar
	$(MAKE) -C plugins
	$(MAKE) -C apps
	$(MAKE) -C gui

clean:
	$(MAKE) -C libtascar clean
	$(MAKE) -C plugins clean
	$(MAKE) -C apps clean
	$(MAKE) -C gui clean
	$(MAKE) -C test clean
	$(MAKE) -C examples clean
	rm -Rf build

test:
	$(MAKE) -C test
	$(MAKE) -C examples

.PHONY : all clean test
