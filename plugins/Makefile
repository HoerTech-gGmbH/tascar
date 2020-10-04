all: build modules

include ../config.mk
include ../rules.mk

#
# main targets:
#
RECEIVERS = omni nsp amb3h0v amb3h3v amb1h0v amb1h1v cardioid	\
  neukom_basic neukom_inphase hann vbap vbap3d hoa2d ortf	\
  intensityvector vmic chmap hoa2d_fuma cardioidmod debugpos	\
  hoa2d_fuma_hos fakebf wfs hoa3d hoa3d_enc simplefdn itu51 foaconv hrtf

SOURCES = omni cardioidmod door generic1storder

TASCARMODS = system pos2osc sampler pendulum epicycles motionpath	\
  foa2hoadiff route oscevents oscjacktime ltcgen lightctl nearsensor	\
  dirgain dummy matrix hoafdnrot mpu6050track touchosc savegains	\
  hrirconv joystick hossustain midicc2osc midictl rotator oscserver	\
  qualisystracker orientationmodulator locationmodulator		\
  locationvelocity granularsynth fail oscrelay waitforjackport sleep	\
  ovheadtracker

TASCARMODSGUI = tracegui simplecontroller timedisplay geopresets

AUDIOPLUGINS = delay dummy gate hannenv identity lipsync							\
  lipsync_paper lookatme metronome onsetdetector sine sndfile spksim	\
  gainramp pulse pink dumplevels feedbackdelay bandpass filter				\
  level2osc const noise loopmachine pos2osc gain sndfileasync					\
  addchannel

ifeq "$(HAS_OPENCV2)" "yes"
GLABSENSORS += trackir
CXXFLAGS += -DHAS_OPENCV2
endif
ifeq "$(HAS_OPENCV4)" "yes"
GLABSENSORS += trackir
CXXFLAGS += -DHAS_OPENCV4
endif
# trackir requires opencv2, which is not available on Ubuntu 20.04


ifeq "$(HAS_LSL)" "yes"
TASCARMODS +=  lsljacktime lslactor levels2osc pos2lsl
AUDIOPLUGINS += speechactivity periodogram
TASCARMODSGUI += datalogging glabsensors
GLABSENSORS += smiley eog emergency espheadtracker midicc	\
  qualisys jackstatus serial
endif

ifeq "$(HAS_OPENMHA)" "yes"
AUDIOPLUGINS +=  openmha
endif

#
RECEIVERMODS = $(patsubst %,build/$(PLUGINPREFIX)tascarreceiver_%.so,$(RECEIVERS))

SOURCEMODS = $(patsubst %,build/$(PLUGINPREFIX)tascarsource_%.so,$(SOURCES))

TASCARMODDLLS = $(patsubst %,build/$(PLUGINPREFIX)tascar_%.so,$(TASCARMODS))
TASCARMODDLLSGUI = $(patsubst %,build/$(PLUGINPREFIX)tascar_%.so,$(TASCARMODSGUI))

AUDIOPLUGINDLLS = $(patsubst %,build/$(PLUGINPREFIX)tascar_ap_%.so,$(AUDIOPLUGINS))

GLABSENSORDLLS = $(patsubst %,build/$(PLUGINPREFIX)glabsensor_%.so,$(GLABSENSORS))


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

TASCARLIB = -ltascar

CXXFLAGS += $(GTKDEF) $(LTRDEF)
# add libtascar/src for main headers, and libtascar/build for version number:
CXXFLAGS += -I../libtascar/src -I../libtascar/build
LDFLAGS += -L../libtascar/build
CXXFLAGS += -DTASCARVERSION="\"$(TASCARVERSION)\""

CXXFLAGS += -fpic
EXTERNALS = jack libxml++-2.6 liblo sndfile

LDLIBS += `pkg-config --libs $(EXTERNALS)`
CXXFLAGS += `pkg-config --cflags $(EXTERNALS)`

LDLIBS += -ldl $(TASCARLIB)

modules: $(RECEIVERMODS) $(SOURCEMODS) $(TASCARMODDLLS) $(TASCARMODDLLSGUI) $(AUDIOPLUGINDLLS) $(GLABSENSORDLLS)

clean:
	rm -Rf *~ src/*~ build doc/html

.PHONY : doc

$(TASCARMODDLLSGUI): EXTERNALS += gtkmm-3.0
$(TASCARMODDLLSGUI): CXXFLAGS+=-I./build

build/$(PLUGINPREFIX)tascarreceiver_hoa2d.so: LDLIBS+=-lfftw3f
build/$(PLUGINPREFIX)tascar_ap_lipsync.so: LDLIBS+=-lfftw3f
build/$(PLUGINPREFIX)tascar_ap_lipsync_paper.so: LDLIBS+=-lfftw3f
build/$(PLUGINPREFIX)tascar_ap_speechactivity.so: LDLIBS+=-llsl
build/$(PLUGINPREFIX)tascar_ap_periodogram.so: LDLIBS+=-llsl
build/$(PLUGINPREFIX)tascar_ap_openmha.so: LDLIBS+=-lopenmha
build/$(PLUGINPREFIX)tascar_ltcgen.so: EXTERNALS+=ltc
build/$(PLUGINPREFIX)tascarreceiver_hoa3d_enc.so: EXTERNALS+=gsl
build/$(PLUGINPREFIX)tascarreceiver_hoa3d.so: EXTERNALS+=gsl eigen3
build/$(PLUGINPREFIX)tascar_hossustain.so: EXTERNALS += fftw3f
build/$(PLUGINPREFIX)tascar_lsljacktime.so build/$(PLUGINPREFIX)tascar_pos2lsl.so build/$(PLUGINPREFIX)tascar_levels2osc.so build/$(PLUGINPREFIX)tascar_lslactor.so: LDLIBS+=-llsl
build/$(PLUGINPREFIX)tascar_lightctl.so build/$(PLUGINPREFIX)tascar_ovheadtracker.so: LDLIBS+=-ltascardmx
build/$(PLUGINPREFIX)tascar_simplecontroller.so: build/simplecontroller_glade.h
build/$(PLUGINPREFIX)tascar_datalogging.so: LDLIBS += -lmatio -llsl
build/$(PLUGINPREFIX)tascar_datalogging.so: build/datalogging_glade.h
build/$(PLUGINPREFIX)tascar_timedisplay.so: build/timedisplay_glade.h

build/$(PLUGINPREFIX)glabsensor_%.so: EXTERNALS += gtkmm-3.0
build/$(PLUGINPREFIX)glabsensor_%.so: LDLIBS += -llsl

ifeq "$(HAS_OPENCV2)" "yes"
build/$(PLUGINPREFIX)glabsensor_trackir.so: EXTERNALS += opencv
endif
ifeq "$(HAS_OPENCV4)" "yes"
build/$(PLUGINPREFIX)glabsensor_trackir.so: EXTERNALS += opencv4
endif

build/glabsensors.o: EXTERNALS += gtkmm-3.0
build/glabsensors.o: build/glabsensors_glade.h
build/$(PLUGINPREFIX)tascar_glabsensors.so: build/glabsensors_glade.h
build/$(PLUGINPREFIX)tascar_glabsensors.so: build/glabsensorplugin.o build/glabsensors_glade.h src/glabsensorplugin.h
build/$(PLUGINPREFIX)tascar_glabsensors.so: LDLIBS += -llsl

build/$(PLUGINPREFIX)tascarreceiver_%.so: src/receivermod_%.cc
	$(CXX) -shared -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

build/$(PLUGINPREFIX)tascarsource_%.so: src/tascarsource_%.cc
	$(CXX) -shared -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

build/$(PLUGINPREFIX)tascar_%.so: src/tascarmod_%.cc
	$(CXX) -shared -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

build/$(PLUGINPREFIX)tascar_ap_%.so: src/tascar_ap_%.cc
	$(CXX) -shared -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

build/$(PLUGINPREFIX)glabsensor_%.so: src/glabsensor_%.cc build/glabsensorplugin.o src/glabsensorplugin.h
	$(CXX) -shared -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

build/%_glade.h: src/%.glade
	cat $< | sed 's/tascarversion/$(TASCARVERSION)/g' > $*_glade
	echo "#include <string>" > $@
	xxd -i $*_glade >> $@
	echo "std::string ui_"$*"((const char*)"$*"_glade,(size_t)"$*"_glade_len);" >> $@
	rm -f $*_glade

build/%.o: src/%.cc
	$(CXX) $(CXXFLAGS) -o $@ -c $^


# Local Variables:
# compile-command: "make"
# End:
