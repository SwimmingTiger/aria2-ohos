#!/bin/sh
set -e -x
# build the project on macOS
# assume deveco studio is downloaded from:
# https://developer.huawei.com/consumer/cn/download/
# and extracted to /Applications/DevEco-Studio.app
export TOOL_HOME=/Applications/DevEco-Studio.app/Contents
export DEVECO_SDK_HOME=$TOOL_HOME/sdk
export PATH=$TOOL_HOME/tools/ohpm/bin:$PATH
export PATH=$TOOL_HOME/tools/hvigor/bin:$PATH
export PATH=$TOOL_HOME/tools/node/bin:$PATH
hvigorw assembleHap
# add hnp, and sign manually
pushd entry
zip -1 -r ../entry/build/default/outputs/default/entry-default-unsigned.hap hnp
popd
python3 sign.py ./entry/build/default/outputs/default/entry-default-unsigned.hap ./entry/build/default/outputs/default/entry-default-signed.hap
