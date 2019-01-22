# variables:
VERSION=0.191.4
CXXFLAGS = -Wall -Wno-deprecated-declarations -msse -msse2 -mfpmath=sse -ffast-math -fno-finite-math-only -fext-numeric-literals -std=c++11 -pthread
PREFIX = /usr/local

GITMODIFIED=$(shell test -z "`git status --porcelain -uno`" || echo "-modified")
COMMITHASH=$(shell git log -1 --abbrev=7 --pretty='format:%h')
FULLVERSION=$(VERSION)-$(COMMITHASH)$(GITMODIFIED)

HAS_LSL=$(shell echo "int main(int,char**){return 0;}"|g++ -llsl -x c++ - 2>/dev/null && echo yes||echo no)

HAS_OPENMHA=$(shell (echo "\#include <openmha/mha_algo_comm.hh>";echo "int main(int,char**){return 0;}")|g++ -lopenmha -x c++ - 2>/dev/null && echo yes||echo no)

BUILD_DIR = build
SOURCE_DIR = src

export VERSION
export SOURCE_DIR
export BUILD_DIR
export CXXFLAGS
export HAS_LSL
export HAS_OPENMHA
