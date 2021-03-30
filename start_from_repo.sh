#!/bin/sh
LOCTSC=$(pwd)/$(dirname $0)
echo $LOCTSC
LOCTSCLIB="${LOCTSC}/libtascar/build:${LOCTSC}/plugins/build"
LD_LIBRARY_PATH="${LOCTSCLIB}" $*
