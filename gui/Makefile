all: build bin

include ../config.mk
include ../rules.mk

TASCARVERSION=$(FULLVERSION)
TASCARVERSION_MAJOR = $(shell echo $(TASCARVERSION)|sed -e 's/\..*//1')
TASCARVERSION_MINOR = $(shell echo $(TASCARVERSION)|sed -e 's/[^\.]*\.//1' -e 's/\..*//1')

CXXFLAGS += -DTASCARVERSION_MAJOR=$(TASCARVERSION_MAJOR) -DTASCARVERSION_MINOR=$(TASCARVERSION_MINOR) -DTASCARVERSION=\"$(TASCARVERSION)\"

BINFILES = build/tascar
OBJECTS = build/scene_manager.o build/tascar_mainwindow.o

GTKMMBIN = build/tascar


ifeq "$(DEBUG)" "yes"
CXXFLAGS += -Wall -g -L./
else
CXXFLAGS += -Wall -O3 -L./
endif

EXTERNALS = alsa jack libxml++-2.6 liblo sndfile gtkmm-3.0 glibmm-2.4 gtksourceviewmm-3.0 libcurl

HAS_WEBKIT40 = $(shell pkg-config webkit2gtk-4.0 && echo "yes"||echo "no")
ifeq "$(HAS_WEBKIT40)" "yes"
EXTERNALS += webkit2gtk-4.0
CXXFLAGS += -DWEBKIT2GTK40=1
else
EXTERNALS += webkit2gtk-3.0
CXXFLAGS += -DWEBKIT2GTK30=1
endif

LDLIBS += -ltascar -ltascargui -ldl -lfftw3f
LDFLAGS += -L../libtascar/build
CXXFLAGS += -I../libtascar/src -I./src -I../libtascar/build -I./build
CPPFLAGS += -I../libtascar/src -I./src -I../libtascar/build -I./build

LDLIBS += `pkg-config --libs $(EXTERNALS)`
CXXFLAGS += `pkg-config --cflags $(EXTERNALS)`

bin: $(BINFILES)

install:
	$(MAKE) -C build -f ../Makefile $(INSTBIN)

uninstall:
	rm -f $(INSTBIN)

clean:
	rm -Rf *~ src/*~ build doc/html

tascarclean:
	$(MAKE) -C ../ clean

$(PREFIX)/bin/%: %
	cp $< $@

build/tascar: $(OBJECTS)

build/%: build/%.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/%.o: src/%.cc
	$(CPP) $(CPPFLAGS) -MM -MF $(@:.o=.mk) $<
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/%_glade.h: src/%.glade
	cat $< | sed 's/tascarversion/$(TASCARVERSION)/g' > $*_glade
	echo "#include <string>" > $@
	xxd -i $*_glade >> $@
	echo "std::string ui_"$*"((const char*)"$*"_glade,(size_t)"$*"_glade_len);" >> $@
	rm -f $*_glade

build/tascar: build/tascar_glade.h

build/tascar_mainwindow.o: build/tascar_xy.xpm build/tascar_xz.xpm build/tascar_yz.xpm build/tascar_xyz.xpm src/tascar_mainwindow.h

build/tascar_%.xpm:
	convert -font Helvetica -size 24x24 -gravity center label:$* -transparent white $@
	sed -i -e 's/static char/static const char/1' $@


# Local Variables:
# compile-command: "make"
# End:
