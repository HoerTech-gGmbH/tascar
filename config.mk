# variables:
VERSION=0.193.0
CXXFLAGS = -Wall -Wno-deprecated-declarations -msse -msse2 -mfpmath=sse -ffast-math -fno-finite-math-only -fext-numeric-literals -std=c++11 -pthread
PREFIX = /usr/local

GITMODIFIED=$(shell test -z "`git status --porcelain -uno`" || echo "-modified")
COMMITHASH=$(shell git log -1 --abbrev=7 --pretty='format:%h')
FULLVERSION=$(VERSION)-$(COMMITHASH)$(GITMODIFIED)

mkfile_name := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_path := $(subst $(notdir $(mkfile_name)),,$(mkfile_name))

HAS_LSL=$(shell $(mkfile_path)/check_for_lsl)

HAS_OPENMHA=$(shell $(mkfile_path)/check_for_openmha)

HAS_OPENCV2=$(shell $(mkfile_path)/check_for_opencv2)

BUILD_DIR = build
SOURCE_DIR = src

export VERSION
export SOURCE_DIR
export BUILD_DIR
export CXXFLAGS
export HAS_LSL
export HAS_OPENMHA
export HAS_OPENCV2
