EXAMPLES = $(shell find -name "*.tsc")

all: $(EXAMPLES:.tsc=.validated)

%.validated: %.tsc
	LD_LIBRARY_PATH=../build/ ../build/tascar_validatetsc $< 
