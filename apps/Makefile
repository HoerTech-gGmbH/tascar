all: build bin

include ../config.mk
include ../rules.mk

#
# main targets:
#
BINFILES = tascar_cli tascar_tscupdate tascar_showlicenses		\
  tascar_validatetsc tascar_pdf tascar_osc_jack_transport		\
  tascar_jackio tascar_sampler tascar_hdspmixer tascar_levelmeter	\
  tascar_jackpar tascar_renderfile tascar_renderir tascar_gpx2csv	\
  tascar_version compare_sndfile

TEST_FILES = test_ngon test_sinc

BINFILES += $(TEST_FILES)

BUILD_BINS = $(patsubst %,build/%,$(BINFILES))

ifeq "$(HAS_LSL)" "yes"
BINFILES += tascar_lslsl tascar_lsljacktime
endif

ifeq "ok" "$(shell pkg-config gtkmm-3.0 && echo ok)"
GTKDEF = "-DGTKMM30"
GTKEXT = gtkmm-3.0
else
GTKDEF = "-DGTKMM24"
GTKEXT = gtkmm-2.4
endif

ifeq "$(DEBUG)" "yes"
CXXFLAGS += -ggdb
else
CXXFLAGS += -O3 
endif

ifeq "$(COVERAGE)" "yes"
CXXFLAGS += -coverage -fprofile-arcs -ftest-coverage -fPIC
endif

CXXFLAGS += $(GTKDEF) $(LTRDEF)
# add libtascar/src for main headers, and libtascar/build for version number:
CXXFLAGS += -I../libtascar/src -I../libtascar/build
CPPFLAGS += -I../libtascar/src -I../libtascar/build
LDFLAGS += -L../libtascar/build 

LDLIBS += -ldl -ltascar -lfftw3f

EXTERNALS = jack libxml++-2.6 liblo sndfile alsa

LDLIBS += `pkg-config --libs $(EXTERNALS)`
CXXFLAGS += `pkg-config --cflags $(EXTERNALS)`

bin: $(BUILD_BINS)

build/tascar_hoadisplay: EXTERNALS += $(GTKEXT)
build/tascar_hdspmixer: EXTERNALS += alsa
build/tascar_gui build/tascar_pdf: LDLIBS += -ltascargui `pkg-config --libs $(EXTERNALS)`
build/tascar_gui build/tascar_pdf: EXTERNALS += $(GTKEXT)
build/tascar_ambdecoder: LDLIBS += `pkg-config --libs gsl`
build/tascar_lslsl build/tascar_lsljacktime: LDLIBS+=-llsl64

ifeq "$(COVERAGE)" "yes"
LDLIBS += -lgcov -lgmock_main
endif


clean:
	rm -Rf *~ src/*~ build doc/html

include $(wildcard build/*.mk)

$(PREFIX)/bin/%: %
	cp $< $@

build/%: build/%.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/%.o: src/%.cc
	$(CPP) $(CPPFLAGS) -MM -MF $(@:.o=.mk) $<
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Local Variables:
# compile-command: "make"
# End:
