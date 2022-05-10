#!/bin/sh
BASE=$(realpath $(dirname $0))
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${BASE}/libtascar/build:${BASE}/plugins/build \
	       DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:${BASE}/libtascar/build:${BASE}/plugins/build \
	       PATH=$PATH:${BASE}/apps/build \
	       ${BASE}/gui/build/tascar_spkcalib $*
