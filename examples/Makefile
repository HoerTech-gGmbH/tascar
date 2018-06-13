EXAMPLES = $(shell find -name "*.tsc")

BUILDPATH=$(shell pwd)/../build/

all: $(EXAMPLES:.tsc=.validated)

%.validated: %.tsc
	LD_LIBRARY_PATH=$(BUILDPATH) ../build/tascar_validatetsc $< 
