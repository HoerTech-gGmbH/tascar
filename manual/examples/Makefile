EXAMPLES = $(shell find . -name "*.tsc")

LIBPATH=$(shell pwd)/../../libtascar/build/
PLUGPATH=$(shell pwd)/../../plugins/build/
BINPATH=$(shell pwd)/../../apps/build/

all: $(EXAMPLES:.tsc=.validated)

%.validated: %.tsc $(BINPATH)tascar_validatetsc $(LIBPATH)libtascar.so
	@LD_LIBRARY_PATH=$(LIBPATH):$(PLUGPATH) $(BINPATH)tascar_validatetsc $< || (echo "$(<):1:";false)
	echo ok > $@

%.jvalid: %.tsc $(BINPATH)tascar_cli $(LIBPATH)libtascar.so
	@LD_LIBRARY_PATH=$(LIBPATH):$(PLUGPATH) $(BINPATH)tascar_cli -v $<  || (echo "$(<):1:";false)
	echo ok > $@

JVALIDTARG = $(filter-out ./example_glabsensor_midicc.jvalid ./example_geopresets.jvalid ./example_qualisys.jvalid,$(EXAMPLES:.tsc=.jvalid))

jackvalid:
	jackd -d dummy -r 44100 -p 1024 &
	sleep 1
	$(MAKE) $(JVALIDTARG) || (killall jackd;false)
	killall jackd

format: $(EXAMPLES:.tsc=.format)

%.format: %.tsc
	xmllint --format $< -o $<
	echo ok > $@

clean:
	rm -f *.validated *.jvalid *.format
