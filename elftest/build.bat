rmdir /s /q build
md build
cd build

set PATH=C:\Program Files\Huawei\DevEco Studio\sdk\default\openharmony\native\build-tools\cmake\bin;%PATH%
set TOOLCHAIN=C:\Program Files\Huawei\DevEco Studio\sdk\default\openharmony\native\build\cmake\ohos.toolchain.cmake

cmake .. -G Ninja -DOHOS_STL=c++_shared -DOHOS_ARCH=arm64-v8a -DOHOS_PLATFORM=OHOS -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN%"

ninja

copy /y /b .\elftest ..\..\entry\libs\arm64-v8a\elftest.so
