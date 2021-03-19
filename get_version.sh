#!/bin/sh
VERSION=$(cat config.mk | sed -e '2 ! d' -e 's/.*=//1')
GITMODIFIED=$(test -z "`git status --porcelain -uno`" || echo "-modified")
COMMITHASH=$(git log -1 --abbrev=7 --pretty='format:%h')
COMMIT_SINCE_MASTER=$(git log --pretty='format:%h' origin/master.. | wc -w | xargs)

FULLVERSION=${VERSION}.${COMMIT_SINCE_MASTER}-${COMMITHASH}${GITMODIFIED}
HAS_LSL=$(sh check_for_lsl)
HAS_OPENMHA=$(sh check_for_openmha)
HAS_OPENCV2=$(sh check_for_opencv2)
HAS_OPENCV4=$(sh check_for_opencv4)

#echo "Creating tascarver.h with version ${FULLVERSION}"
mkdir -p libtascar/build
echo '#define TASCARVER "'$(FULLVERSION)'"' > libtascar/build/tascarver.h

