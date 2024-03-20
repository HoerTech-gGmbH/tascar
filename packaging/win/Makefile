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
	(cd tascar; cp -v $$(cygcheck ./* | sort -bu) .;mkdir -p share/{icons,glib-2.0} lib); \
	cp -r /d/*/*/*/*/share/icons/Adwaita tascar/share/icons;\
	cp -r /d/*/*/*/*/share/glib-2.0/schemas tascar/share/glib-2.0;\
	cp -r /mingw64/lib/gdk-pixbuf-2.0 tascar/lib;\
	cp -r ../../scripts/*.m tascar/scripts;\
	cp /d/*/*/*/*/bin/gdbus.exe tascar;\
	cp /d/*/*/*/*/bin/broadwayd.exe tascar;\
	cp ../../artwork/*.png tascar;\
	zip -r tascar-$(PACKVERSION).zip tascar

ver:
	@echo "Packaging: $(PACKVERSION)"
