#!/bin/bash
test -e tascar || git clone https://github.com/gisogrimm/tascar
cd tascar
git pull origin master
make -j 4
