all: build modules

include ../config.mk
include ../rules.mk

#
# main targets:
#
RECEIVERS = omni nsp amb3h0v amb3h3v amb1h0v amb1h1v cardioid	\
  neukom_basic neukom_inphase hann vbap vbap3d hoa2d ortf	\
  intensityvector vmic chmap hoa2d_fuma cardioidmod debugpos	\
  hoa2d_fuma_hos fakebf

SOURCES = omni cardioidmod door

TASCARMODS = system pos2osc sampler pendulum epicycles motionpath	\
  foa2hoadiff route oscevents oscjacktime ltcgen lightctl nearsensor	\
  dirgain dummy matrix hoafdnrot mpu6050track touchosc savegains	\
  hrirconv joystick hossustain midicc2osc midictl rotator

TASCARMODSGUI = tracegui simplecontroller timedisplay geopresets

AUDIOPLUGINS = delay dummy gate hannenv identity lipsync		\
  lipsync_paper lookatme metronome onsetdetector sine sndfile spksim	\
  gainramp pulse pink dumplevels

ifeq "$(HAS_LSL)" "yes"
TASCARMODS +=  lsljacktime lslactor levels2osc pos2lsl
AUDIOPLUGINS += speechactivity
TASCARMODSGUI += datalogging
endif

ifeq "$(HAS_OPENMHA)" "yes"
AUDIOPLUGINS +=  openmha
endif

#
RECEIVERMODS = $(patsubst %,build/tascarreceiver_%.so,$(RECEIVERS))

SOURCEMODS = $(patsubst %,build/tascarsource_%.so,$(SOURCES))

TASCARMODDLLS = $(patsubst %,build/tascar_%.so,$(TASCARMODS))
TASCARMODDLLSGUI = $(patsubst %,build/tascar_%.so,$(TASCARMODSGUI))

AUDIOPLUGINDLLS = $(patsubst %,build/tascar_ap_%.so,$(AUDIOPLUGINS))

ifeq "ok" "$(shell pkg-config gtkmm-3.0 && echo ok)"
GTKDEF = "-DGTKMM30"
GTKEXT = gtkmm-3.0
else
GTKDEF = "-DGTKMM24"
GTKEXT = gtkmm-2.4
endif

ifeq "$(DEBUG)" "yes"
CXXFLAGS += -ggdb -DTSCDEBUG
else
CXXFLAGS += -O3 
endif

TASCARVERSION=$(FULLVERSION)

CXXFLAGS += $(GTKDEF) $(LTRDEF)
# add libtascar/src for main headers, and libtascar/build for version number:
CXXFLAGS += -I../libtascar/src -I../libtascar/build
LDFLAGS += -L../libtascar/build
CXXFLAGS += -DTASCARVERSION="\"$(TASCARVERSION)\""

CXXFLAGS += -fpic
EXTERNALS = jack libxml++-2.6 liblo sndfile

LDLIBS += `pkg-config --libs $(EXTERNALS)`
CXXFLAGS += `pkg-config --cflags $(EXTERNALS)`

LDLIBS += -ldl -ltascar

modules: $(RECEIVERMODS) $(SOURCEMODS) $(TASCARMODDLLS) $(TASCARMODDLLSGUI) $(AUDIOPLUGINDLLS)

clean:
	rm -Rf *~ src/*~ build doc/html

.PHONY : doc

$(TASCARMODDLLSGUI): EXTERNALS += gtkmm-3.0
$(TASCARMODDLLSGUI): CXXFLAGS+=-I./build

build/tascarreceiver_hoa2d.so: LDLIBS+=-lfftw3f
build/tascar_ap_lipsync.so: LDLIBS+=-lfftw3f
build/tascar_ap_lipsync_paper.so: LDLIBS+=-lfftw3f
build/tascar_ap_speechactivity.so: LDLIBS+=-llsl64
build/tascar_ap_openmha.so: LDLIBS+=-lopenmha
build/tascar_ltcgen.so: EXTERNALS+=ltc
build/tascar_hossustain.so: EXTERNALS += fftw3f
build/tascar_lsljacktime.so build/tascar_pos2lsl.so build/tascar_levels2osc.so build/tascar_lslactor.so: LDLIBS+=-llsl64
build/tascar_lightctl.so: LDLIBS+=-ltascardmx
build/tascar_simplecontroller.so: build/simplecontroller_glade.h
build/tascar_datalogging.so: LDLIBS += -lmatio -llsl64
build/tascar_datalogging.so: build/datalogging_glade.h
build/tascar_timedisplay.so: build/timedisplay_glade.h

build/tascarreceiver_%.so: src/receivermod_%.cc
	$(CXX) -shared -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

build/tascarsource_%.so: src/tascarsource_%.cc
	$(CXX) -shared -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

build/tascar_%.so: src/tascarmod_%.cc
	$(CXX) -shared -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

build/tascar_ap_%.so: src/tascar_ap_%.cc
	$(CXX) -shared -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

build/%_glade.h: src/%.glade
	cat $< | sed 's/tascarversion/$(TASCARVERSION)/g' > $*_glade
	echo "#include <string>" > $@
	xxd -i $*_glade >> $@
	echo "std::string ui_"$*"((const char*)"$*"_glade,(size_t)"$*"_glade_len);" >> $@
	rm -f $*_glade


# Local Variables:
# compile-command: "make"
# End:
