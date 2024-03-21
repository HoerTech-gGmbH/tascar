include ../../config.mk

ALL_CSV=$(wildcard *.csv)

PACKVERSION=$(FULLVERSION)-windows
ifeq "$(DEBUG)" "yes"
PACKVERSION:=$(PACKVERSION)-debug
endif

all:
	rm -Rf tascar;\
	mkdir -p tascar/scripts;\
	rm -f tascar-$(PACKVERSION).zip;\
	cp -v ../../*/build/[lt]*.{dll,exe} tascar;\
	cp -r ../../scripts/*.m tascar/scripts;\
	cp -r /d/*/*/*/*/share/icons/Adwaita tascar/share/icons;\
	cp -r /d/*/*/*/*/share/glib-2.0/schemas tascar/share/glib-2.0;\
	cp -r /d/*/*/*/*/lib/gdk-pixbuf-2.0 tascar/lib;\
	cp /d/*/*/*/*/bin/gdbus.exe tascar;\
	cp /d/*/*/*/*/bin/broadwayd.exe tascar;\
	(cd tascar; cp -v $$(cygcheck ./* ./lib/gdk-pixbuf-2.0/*/*.dll | grep msys64 | sort -bu) .;mkdir -p share/{icons,glib-2.0} lib); \
	cp ../../artwork/*.png tascar;\
	zip -r tascar-$(PACKVERSION).zip tascar; \
	(cd tascar; cygcheck ./*; find /d/ -name "Adwaita*"; find /d/ -name "glib-2.0"; find /d/ -name "schemas";echo "";)


ver:
	@echo "Packaging: $(PACKVERSION)"
