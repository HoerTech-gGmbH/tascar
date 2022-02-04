EXAMPLES = $(shell find -name "*.tsc")

LIBPATH=$(shell pwd)/../libtascar/build/
PLUGPATH=$(shell pwd)/../plugins/build/
BINPATH=$(shell pwd)/../apps/build/

all: $(EXAMPLES:.tsc=.validated)

%.validated: %.tsc $(BINPATH)tascar_validatetsc $(LIBPATH)libtascar.so
	@LD_LIBRARY_PATH=$(LIBPATH):$(PLUGPATH) $(BINPATH)tascar_validatetsc $< || (echo "$(<):1:";false)
	echo ok > $@

%.jvalid: %.tsc $(BINPATH)tascar_cli $(LIBPATH)libtascar.so
	@LD_LIBRARY_PATH=$(LIBPATH):$(PLUGPATH) $(BINPATH)tascar_cli -v $<  || (echo "$(<):1:";false)
	echo ok > $@

jackvalid:
	jackd -d dummy -C 64 -P 64 -r 44100 -p 1024 &
	sleep 1
	$(MAKE) $(EXAMPLES:.tsc=.jvalid) || (killall jackd;false)
	killall jackd

clean:
	rm -f *.jvalid */*.jvalid *.validated */*.validated

