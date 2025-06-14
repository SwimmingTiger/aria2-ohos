include ../utils/Makefrag

all: download/expat-2.7.1.tar.xz
	rm -rf temp build
	mkdir -p temp build
	cd temp && tar xvf ../download/expat-2.7.1.tar.xz
	cd temp/expat-2.7.1 && mkdir build
	cd temp/expat-2.7.1/build && ../configure --disable-static --enable-shared --without-zstd --host aarch64-unknown-linux-musl
	cd temp/expat-2.7.1/build && make -j $(shell nproc)
	cd temp/expat-2.7.1/build && make install DESTDIR=$(shell pwd)/build
	mkdir -p ../sysroot
	rm -v build/usr/local/lib/*.la
	cp -rfv build/usr/local/. ../sysroot | tee file.lst

download/expat-2.7.1.tar.xz:
	mkdir -p download
	cd download && wget https://github.com/libexpat/libexpat/releases/download/R_2_7_1/expat-2.7.1.tar.xz
