#!/bin/sh
set -e -x
export LC_CTYPE=C
export TOOL_HOME=/Applications/DevEco-Studio.app/Contents
find . -name '*.hnp' -delete
make -C build-hnp
