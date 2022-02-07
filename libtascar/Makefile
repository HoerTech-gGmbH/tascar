all: build ver lib symlink

include ../config.mk
include ../rules.mk

#
# versioning:
#
LIBVERSION=$(VERSION)
SOVERSION=$(shell echo $(VERSION) | sed 's/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\)/\1.\2/1')
DYLIBCURVERSION=$(shell echo $(VERSION) | sed 's/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\)/\1.\2.\3/1')

#
# main targets:
#
OBJECTS = licensehandler.o audiostates.o coordinates.o audiochunks.o	\
  tscconfig.o dynamicobjects.o sourcemod.o receivermod.o		\
  filterclass.o osc_helper.o pluginprocessor.o acousticmodel.o		\
  scene.o render.o session_reader.o session.o jackclient.o		\
  delayline.o async_file.o errorhandling.o osc_scene.o ringbuffer.o	\
  sampler.o jackiowav.o cli.o irrender.o jackrender.o	\
  audioplugin.o maskplugin.o levelmeter.o serviceclass.o		\
  speakerarray.o spectrum.o fft.o stft.o ola.o vbap3d.o hoa.o		\
  tascar_os.o
# pugixml.o

ifneq ($(OS),Windows_NT)
UNAME_S := $(shell uname -s)
OBJECTS += spawn_process.o
ifeq ($(UNAME_S),Linux)
  OBJECTS +=  alsamidicc.o
endif
endif

DMXOBJECTS =  termsetbaud.o serialport.o dmxdriver.o

GUIOBJECTS = gui_elements.o pdfexport.o viewport.o 

# build dir:
BUILD_OBJECTS = $(patsubst %,build/%,$(OBJECTS))
BUILD_DMXOBJECTS = $(patsubst %,build/%,$(DMXOBJECTS))
BUILD_GUIOBJECTS = $(patsubst %,build/%,$(GUIOBJECTS))

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

ifeq "$(COVERAGE)" "yes"
CXXFLAGS += -coverage -fprofile-arcs -ftest-coverage
LDLIBS += -lgcov
endif

CXXFLAGS += $(GTKDEF) $(LTRDEF) $(TSCCXXFLAGS)

CXXFLAGS += -fpic -I./build -I./include
CPPFLAGS += -I./build -I./include
#-L./ 
EXTERNALS = jack liblo sndfile fftw3f gsl eigen3 samplerate xerces-c

ifneq ($(OS),Windows_NT)
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
  EXTERNALS +=  alsa
endif
else
  EXTERNALS += regex
endif


$(BUILD_GUIOBJECTS): EXTERNALS += gtkmm-3.0

LDLIBS += `pkg-config --libs $(EXTERNALS)`
CXXFLAGS += `pkg-config --cflags $(EXTERNALS)`

LDLIBS += -ldl
LDFLAGS += -L./

ver:
	echo '#define TASCARVER "'$(FULLVERSION)'"' > build/tascarver.h

ifeq ($(OS),Windows_NT)
LDLIBS += -lshlwapi -lssp
lib:  $(BUILD_OBJECTS) build/lib$(PLUGINPREFIX)tascar-$(LIBVERSION).dll build/lib$(PLUGINPREFIX)tascargui-$(LIBVERSION).dll
symlink: build/lib$(PLUGINPREFIX)tascar.dll build/lib$(PLUGINPREFIX)tascargui.dll
else
ifeq ($(UNAME_S),Linux)
lib:  $(BUILD_OBJECTS) build/lib$(PLUGINPREFIX)tascar.so.$(LIBVERSION) build/lib$(PLUGINPREFIX)tascargui.so.$(LIBVERSION) build/lib$(PLUGINPREFIX)tascardmx.so.$(LIBVERSION)
	ldconfig -n build
symlink: build/lib$(PLUGINPREFIX)tascar.so build/lib$(PLUGINPREFIX)tascargui.so build/lib$(PLUGINPREFIX)tascardmx.so
else
# probably on MacOS
lib:   $(BUILD_OBJECTS) build/lib$(PLUGINPREFIX)tascar.0.dylib build/lib$(PLUGINPREFIX)tascargui.0.dylib
symlink: build/lib$(PLUGINPREFIX)tascar.dylib build/lib$(PLUGINPREFIX)tascargui.dylib
endif
endif

build/lib$(PLUGINPREFIX)tascar.so.%: $(BUILD_OBJECTS)
	$(CXX) -shared -Wl,-soname,lib$(PLUGINPREFIX)tascar.so.$(SOVERSION) -o $@ $^ ${LDFLAGS} ${LDLIBS}
	rm -f build/lib$(PLUGINPREFIX)tascar.so

build/lib$(PLUGINPREFIX)tascargui.so.%: EXTERNALS += gtkmm-3.0
build/lib$(PLUGINPREFIX)tascargui.so.%: $(BUILD_GUIOBJECTS)
	$(CXX) -shared -Wl,-soname,lib$(PLUGINPREFIX)tascargui.so.$(SOVERSION) -o $@ $^ ${LDFLAGS} ${LDLIBS}
	rm -f build/lib$(PLUGINPREFIX)tascargui.so

build/lib$(PLUGINPREFIX)tascardmx.so.%: $(BUILD_DMXOBJECTS)
	$(CXX) -shared -Wl,-soname,lib$(PLUGINPREFIX)tascardmx.so.$(SOVERSION) -o $@ $^ ${LDFLAGS} ${LDLIBS}
	rm -f build/lib$(PLUGINPREFIX)tascardmx.so

build/lib$(PLUGINPREFIX)tascar-$(LIBVERSION).dll: $(BUILD_OBJECTS)
	$(CXX) -shared -Wl,-soname,lib$(PLUGINPREFIX)tascar.so.$(SOVERSION) -o $@ $^ ${LDFLAGS} ${LDLIBS}

build/lib$(PLUGINPREFIX)tascar.0.dylib: $(BUILD_OBJECTS)
	$(CXX) -dynamiclib -install_name lib$(PLUGINPREFIX)tascar.0.dylib -current_version $(DYLIBCURVERSION) -compatibility_version $(SOVERSION) -o $@ $^ ${LDFLAGS} ${LDLIBS}

build/lib$(PLUGINPREFIX)tascargui.0.dylib: EXTERNALS += gtkmm-3.0
build/lib$(PLUGINPREFIX)tascargui.0.dylib: $(BUILD_GUIOBJECTS) build/lib$(PLUGINPREFIX)tascar.0.dylib
	$(CXX) -dynamiclib -install_name lib$(PLUGINPREFIX)tascargui.0.dylib -current_version $(DYLIBCURVERSION) -compatibility_version $(SOVERSION) -o $@ $^ ${LDFLAGS} ${LDLIBS}

build/lib$(PLUGINPREFIX)tascargui-$(LIBVERSION).dll: EXTERNALS += gtkmm-3.0
build/lib$(PLUGINPREFIX)tascargui-$(LIBVERSION).dll: $(BUILD_GUIOBJECTS) build/lib$(PLUGINPREFIX)tascar-$(LIBVERSION).dll
	$(CXX) -shared -Wl,-soname,lib$(PLUGINPREFIX)tascargui.so.$(SOVERSION) -o $@ $^ ${LDFLAGS} ${LDLIBS}

clean:
	rm -Rf *~ src/*~ build doc/html

.PHONY : doc test gui

doc:
	cd doc && sed -e 's/PROJECT.NUMBER.*=.*/&'`cat ../version`'/1' doxygen.cfg > .temp.cfg && doxygen .temp.cfg
	rm -Rf doc/.temp.cfg

#include $(BUILD_OBJECTS:.o=.mk)

build/%.mk: src/%.cc
	$(CPP) $(CXXFLAGS) -MM -MF $(@:.o=.mk) $<
	printf "build/" > $(@).x
	cat $@ >> $(@).x
	mv $(@).x $@

%: %.o
	$(CXX) $(CXXFLAGS) $(LDLIBS) $^ -o $@

build/%.o: src/%.cc include/%.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/pugixml.o: ../external_libs/pugixml-1.11.4/src/pugixml.cpp ../external_libs/pugixml-1.11.4/src/pugixml.hpp ../external_libs/pugixml-1.11.4/src/pugiconfig.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/%.so: build/%.so.$(LIBVERSION)
	rm -f $@; ln -s $(subst build/,,$<) $@

build/%.dll: build/%-$(LIBVERSION).dll
	rm -f $@; ln -s $(subst build/,,$<) $@

build/%.dylib: build/%.0.dylib
	rm -f $@; ln -s $(subst build/,,$<) $@

fullversion:
	echo $(FULLVERSION) >$@

# Local Variables:
# compile-command: "make"
# End:
