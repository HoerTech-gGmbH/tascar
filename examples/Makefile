EXAMPLES = $(shell find -name "*.tsc")

BUILDPATH=$(shell pwd)/../build/

all: $(EXAMPLES:.tsc=.validated)

%.validated: %.tsc $(BUILDPATH)tascar_validatetsc $(BUILDPATH)libtascar.so
	@LD_LIBRARY_PATH=$(BUILDPATH) $(BUILDPATH)tascar_validatetsc $< || (echo "$(<):1:";false)
	echo ok > $@

%.jvalid: %.tsc $(BUILDPATH)tascar_cli $(BUILDPATH)libtascar.so
	@LD_LIBRARY_PATH=$(BUILDPATH) $(BUILDPATH)tascar_cli -v $<  || (echo "$(<):1:";false)
	echo ok > $@

jackvalid:
	jackd -d dummy -C 64 -P 64 -r 44100 -p 1024 &
	sleep 1
	$(MAKE) $(EXAMPLES:.tsc=.jvalid) || (killall jackd;false)
	killall jackd
