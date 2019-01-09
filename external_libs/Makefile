all: build

BUILD_DIR = build

include ../config.mk
include ../rules.mk

clean:
	rm -Rf googlemock googletest $(BUILD_DIR)

googlemock: $(BUILD_DIR)/googlemock.is_installed

$(BUILD_DIR)/googlemock.is_installed: googletest/WORKSPACE \
	$(BUILD_DIR)/lib/.directory $(BUILD_DIR)/include/.directory
	echo $(CXXFLAGS)
	cd googletest/googlemock/make && $(MAKE)
	cp googletest/googlemock/make/gmock_main.a $(BUILD_DIR)/lib/libgmock_main.a
	cp -a googletest/googletest/include/gtest $(BUILD_DIR)/include/
	cp -a googletest/googlemock/include/gmock $(BUILD_DIR)/include/
	touch $@

googletest/WORKSPACE:
	git clone https://github.com/google/googletest
	(cd googletest && git checkout 31eb5e9b873af4b509be2f77616113007fa0de9d)
