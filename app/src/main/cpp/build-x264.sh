#! /bin/bash

ROOT=$(pwd)
X264ROOT=$ROOT/x264

cd "$X264ROOT"

CROSS_PREFIX=$NDK/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android-
SYSROOT=$NDK/toolchains/llvm/prebuilt/darwin-x86_64/sysroot
TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/darwin-x86_64/bin
#export CC=$TOOLCHAIN/aarch64-linux-android21-clang
#export CXX=$TOOLCHAIN/aarch64-linux-android21-clang++
#export AS=$TOOLCHAIN/aarch64-linux-android21-clang
#export LD=$TOOLCHAIN/aarch64-linux-android21-clang
export AR=$TOOLCHAIN/aarch64-linux-android-ar

CPU=armv8-a
ARCH=arm64

CFLAGS="-fstack-protector"
CFLAGS+=" -fstrict-aliasing"
CFLAGS+=" -O2"
LDFLAGS=" -march=${CPU}"

CLANG_TARGET=aarch64-none-linux-android21

if [ ! -e ${CROSS_PREFIX}ar ]; then
  CROSS_PREFIX=${TOOLCHAIN}/llvm-
  export AR=$TOOLCHAIN/llvm-ar
  fi

	# Clang only.
	export CC="$TOOLCHAIN/clang -target $CLANG_TARGET"
	export AS="$TOOLCHAIN/clang -target $CLANG_TARGET"
	export LD="$TOOLCHAIN/clang -target $CLANG_TARGET"
	export CXX="$TOOLCHAIN/clang++ -target $CLANG_TARGET"

	#EXTRA_CFLAGS+="-target $CLANG_TARGET -gcc-toolchain $TOOLCHAIN"
	EXTRA_CFLAGS+=" -Wno-deprecated-declarations -Wno-unused-variable -Wno-unused-function"

PREFIX=$X264ROOT/install
./configure \
--prefix="$PREFIX" \
--disable-cli \
--enable-static \
--disable-opencl \
--disable-gpl \
--enable-pic \
--host=aarch64-linux \
--bit-depth=8 \
--extra-cflags="$CFLAGS" \
--extra-ldflags="$LDFLAGS" \
--chroma-format=420 \
--cross-prefix="$CROSS_PREFIX" \
--sysroot="$SYSROOT" \


make -j 4

make install

cd "$ROOT"