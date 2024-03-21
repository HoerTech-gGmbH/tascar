include ../../config.mk

ALL_CSV=$(wildcard *.csv)

PACKVERSION=$(FULLVERSION)-windows
ifeq "$(DEBUG)" "yes"
PACKVERSION:=$(PACKVERSION)-debug
endif

all:
	rm -Rf tascar;\
	mkdir -p tascar/{scripts,share/icons,share/glib-2.0,lib};\
	rm -f tascar-$(PACKVERSION).zip;\
	cp -v ../../*/build/[lt]*.{dll,exe} tascar/;\
	cp -r ../../scripts/*.m tascar/scripts/;\
	cp -r /ucrt64/share/icons/Adwaita tascar/share/icons/;\
	cp -r /ucrt64/share/glib-2.0/schemas tascar/share/glib-2.0/;\
	cp -r /ucrt64/lib/gdk-pixbuf-2.0 tascar/lib/;\
	cp /ucrt64/bin/gdbus.exe tascar/;\
	cp /ucrt64/bin/broadwayd.exe tascar/;\
	(cd tascar; cp -v $$(cygcheck ./* ./lib/gdk-pixbuf-2.0/*/*.dll | grep msys64 | sort -bu) .;mkdir -p share/{icons,glib-2.0} lib); \
	cp ../../artwork/*.png tascar/;\
	(cd tascar; glib-compile-schemas share/glib-2.0/schemas);\
	zip -r tascar-$(PACKVERSION).zip tascar/; \
	(cd tascar; find /d/ -name "Adwaita*"; find /d/ -name "glib-2.0"; find /d/ -name "hicolor"; find /d/ -name "librsvg-2-2.dll";echo "---";echo "---";ls /ucrt64/;echo "packaging completed.";)


ver:
	@echo "Packaging: $(PACKVERSION)"
