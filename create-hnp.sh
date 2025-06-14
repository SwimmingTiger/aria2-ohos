#!/bin/sh
set -e -x
export LC_CTYPE=C
export TOOL_HOME=/Applications/DevEco-Studio.app/Contents
make -C build-hnp
