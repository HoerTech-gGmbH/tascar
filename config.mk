# variables:
VERSION=0.190.0
CXXFLAGS = -Wall -Wno-deprecated-declarations -msse -msse2 -mfpmath=sse -ffast-math -fno-finite-math-only -fext-numeric-literals -std=c++11 -pthread
PREFIX = /usr/local

GITMODIFIED=$(shell test -z "`git status --porcelain -uno`" || echo "-modified")
COMMITHASH=$(shell git log -1 --abbrev=7 --pretty='format:%h')
FULLVERSION=$(VERSION)-$(COMMITHASH)$(GITMODIFIED)

BUILD_DIR = build
SOURCE_DIR = src

export VERSION
export SOURCE_DIR
export BUILD_DIR
export CXXFLAGS
