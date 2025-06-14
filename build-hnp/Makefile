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

cn-hu60-aria2.hnp: $(STAMP) utils/pbcopy utils/pbpaste
	# reduce size
	rm -rfv sysroot/share/man
	rm -rfv sysroot/share/doc
	rm -rfv sysroot/share/info
	# add pasteboard utilities
	cp utils/pbcopy sysroot/bin
	cp utils/pbpaste sysroot/bin
	# let gcc work out of box
	mkdir -p sysroot/aarch64-unknown-linux-musl/lib
	cp $(TOOL_HOME)/sdk/default/openharmony/native/sysroot/usr/lib/aarch64-linux-ohos/crt1.o sysroot/aarch64-unknown-linux-musl/lib/crt1.o
	cp $(TOOL_HOME)/sdk/default/openharmony/native/sysroot/usr/lib/aarch64-linux-ohos/crti.o sysroot/aarch64-unknown-linux-musl/lib/crti.o
	cp $(TOOL_HOME)/sdk/default/openharmony/native/sysroot/usr/lib/aarch64-linux-ohos/crtn.o sysroot/aarch64-unknown-linux-musl/lib/crtn.o
	cp $(TOOL_HOME)/sdk/default/openharmony/native/llvm/lib/clang/15.0.4/lib/aarch64-linux-ohos/clang_rt.crtbegin.o sysroot/aarch64-unknown-linux-musl/lib/crtbegin.o
	cp $(TOOL_HOME)/sdk/default/openharmony/native/llvm/lib/clang/15.0.4/lib/aarch64-linux-ohos/clang_rt.crtend.o sysroot/aarch64-unknown-linux-musl/lib/crtend.o
	cp $(TOOL_HOME)/sdk/default/openharmony/native/llvm/lib/clang/15.0.4/lib/aarch64-linux-ohos/libclang_rt.builtins.a sysroot/aarch64-unknown-linux-musl/lib/libgcc.a
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
