PKGS=\
	c-ares \
	expat \
	openssl \
	aria2
STAMP=$(patsubst %,%/.stamp,$(PKGS))

all: copy

copy: cn-hu60-aria2.hnp
	rm -f ../entry/hnp/arm64-v8a/*.hnp
	mkdir -p ../entry/hnp/arm64-v8a
	cp $^ ../entry/hnp/arm64-v8a

cn-hu60-aria2.hnp: $(STAMP)
	# reduce size
	rm -rfv sysroot/share/man
	rm -rfv sysroot/share/doc
	rm -rfv sysroot/share/info
	rm -rfv sysroot/include
	rm -rfv sysroot/lib/cmake
	rm -rfv sysroot/lib/pkgconfig
	rm -rfv sysroot/bin/adig
	rm -rfv sysroot/bin/ahost
	rm -rfv sysroot/bin/c_rehash
	rm -rfv sysroot/bin/openssl
	rm -rfv sysroot/bin/xmlwf
	# $(TOOL_HOME)/sdk/default/openharmony/toolchains/hnpcli pack -i sysroot -n base -v 1.0
	cp hnp.json sysroot
	rm -f cn-hu60-aria2.hnp
	zip -r cn-hu60-aria2.hnp sysroot

%/.stamp: %/Makefile
	make -C $(patsubst %/.stamp,%,$@)
	touch $@

clean:
	rm -f $(STAMP)
	rm -rf sysroot
