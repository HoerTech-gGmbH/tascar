ARCH = $(shell uname -m)
PLATFORM = $(shell uname -s)
ifeq "$(PLATFORM)" "Linux"
PLATFORM=linux
endif
TASCARVERSION=$(shell sed -e 's/.*TASCARVER //1' -e 's/[^[:alnum:].-]*//g' ../../build/tascarver.h)
TASCARVERSION_MAJOR = $(shell echo $(TASCARVERSION)|sed -e 's/\..*//1')
TASCARVERSION_MINOR = $(shell echo $(TASCARVERSION)|sed -e 's/[^\.]*\.//1' -e 's/\..*//1')

CXXFLAGS += -DTASCARVERSION_MAJOR=$(TASCARVERSION_MAJOR) -DTASCARVERSION_MINOR=$(TASCARVERSION_MINOR)

PREFIX = /usr/local

BINFILES = tascar
OBJECTS = scene_manager.o tascar_mainwindow.o
GUIOBJ = 

INSTBIN = $(patsubst %,$(PREFIX)/bin/%,$(BINFILES))

JACKBIN = 
LOBIN = 
ALSABIN = 
GTKMMBIN = tascar

ifeq "$(ARCH)" "x86_64"
CXXFLAGS += -msse -msse2 -mfpmath=sse -ffast-math -fomit-frame-pointer -fno-finite-math-only
endif

ifeq "$(DEBUG)" "yes"
CXXFLAGS += -Wall -g -L./
else
CXXFLAGS += -Wall -O3 -L./
endif


EXTERNALS = alsa jack libxml++-2.6 liblo sndfile

LDLIBS += -ltascar -ltascargui -ldl -lfftw3f
LDFLAGS += -L../../build
CXXFLAGS += -I../../src -I.
CPPFLAGS += -I../../src

LDLIBS += `pkg-config --libs $(EXTERNALS)`
CXXFLAGS += `pkg-config --cflags $(EXTERNALS)`

all: libtascar
	mkdir -p build
	$(MAKE) -C build -f ../Makefile $(BINFILES)

#$(BINFILES): ../../build/libtascar.a

libtascar:
	$(MAKE) -C ../ lib

install:
	$(MAKE) -C build -f ../Makefile $(INSTBIN)

uninstall:
	rm -f $(INSTBIN)

clean:
	rm -Rf *~ src/*~ build doc/html

tascarclean:
	$(MAKE) -C ../ clean

VPATH = ../src

$(PREFIX)/bin/%: %
	cp $< $@

tascar: $(OBJECTS)

%: %.o
	$(CXX) $(CXXFLAGS) $(LDLIBS) $^ -o $@

%.o: %.cc
	$(CPP) $(CPPFLAGS) -MM -MF $(@:.o=.mk) $<
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(GTKMMBIN): $(GUIOBJ)
$(GTKMMBIN): EXTERNALS += gtkmm-3.0 glibmm-2.4


%_glade.h: %.glade
	cat $< | sed 's/tascarversion/$(TASCARVERSION)/g' > $*_glade
	echo "#include <string>" > $@
	xxd -i $*_glade >> $@
	echo "std::string ui_"$*"((const char*)"$*"_glade,(size_t)"$*"_glade_len);" >> $@

tascar: tascar_glade.h

tascar_mainwindow.o: tascar_xy.xpm tascar_xz.xpm tascar_yz.xpm

tascar_%.xpm:
	convert -font Helvetica -size 24x24 -gravity center label:$* -transparent white $@
	sed -i -e 's/static char/static const char/1' $@

#	convert -size 48x48 -page 48x48 -font helvetica -transparent white label:$^ $@

# Local Variables:
# compile-command: "make"
# End:
