#!/bin/sh
# push and install the hap
# assume deveco studio is downloaded from:
# https://developer.huawei.com/consumer/cn/download/
# and extracted to /Applications/DevEco-Studio.app

if [ "$1" = "" ]; then
    echo "Usage: $0 ./entry/build/default/outputs/default/entry-default-signed.hap"
    exit 1
fi

set -x -e
export TOOL_HOME=/Applications/DevEco-Studio.app/Contents
export PATH=$TOOL_HOME/sdk/default/openharmony/toolchains:$PATH

hdc file send $1 /data/local/tmp
hdc shell bm uninstall -n $(jq ".app.bundleName" AppScope/app.json5)
hdc shell bm install -p /data/local/tmp/$(basename $1)
hdc shell aa start -a EntryAbility -b $(jq ".app.bundleName" AppScope/app.json5)
