#!/bin/bash

NDK=C:/android/android-ndk-r8d
NDK64=C:/android/android-ndk-r12b

TEMPDIR=C:/Temp
TMP=$TEMPDIR

#need to change ffmpeg version for build
SRC_DIR_NAME=ffmpeg-3.x.x

RESULT_DIR=$(pwd)/lib
rm -r $RESULT_DIR
mkdir -p $RESULT_DIR


OPENSSL_DIR=$(pwd)/openssl

echo "======== environment =========="
echo "NDK : $NDK"
echo "NDK64 : $NDK64"
echo "TEMPDIR : $TEMPDIR"
echo "SRC_DIR_NAME : $SRC_DIR_NAME"

function build {
	echo ""
	echo "======== start build : $1 =========="
	rm -rf ${SRC_DIR_NAME}
	tar zxf ${SRC_DIR_NAME}.tar.gz

	case $1 in
		armv5te)
			SYSROOT=$NDK/platforms/android-14/arch-arm
			TOOLCHAIN=$NDK/toolchains/arm-linux-androideabi-4.4.3/prebuilt/windows
			TOOLCHAIN_PREFIX=arm-linux-androideabi
			GCC=$TOOLCHAIN/lib/gcc/$TOOLCHAIN_PREFIX/4.4.3/libgcc.a
			ARCH=arm
			CPU=armv5te
			ANDROID_ABI=armeabi
			ADDI_CFLAGS="-marm -msoft-float -mtune=xscale"
			ADDI_LDFLAGS=""
		;;
		armv7-a)
			SYSROOT=$NDK/platforms/android-14/arch-arm
			TOOLCHAIN=$NDK/toolchains/arm-linux-androideabi-4.4.3/prebuilt/windows
			TOOLCHAIN_PREFIX=arm-linux-androideabi
			GCC=$TOOLCHAIN/lib/gcc/$TOOLCHAIN_PREFIX/4.4.3/libgcc.a
			ARCH=arm
			CPU=armv7-a
			ANDROID_ABI=armeabi-v7a
			ADDI_CFLAGS="-marm -msoft-float -mfloat-abi=softfp -mfpu=neon -mtune=cortex-a8 -mvectorize-with-neon-quad"
			ADDI_LDFLAGS="-Wl,--fix-cortex-a8"
		;;
		armv8-a)
		 	SYSROOT=$NDK64/platforms/android-21/arch-arm64
		 	TOOLCHAIN=$NDK64/toolchains/aarch64-linux-android-4.9/prebuilt/windows-x86_64
		 	TOOLCHAIN_PREFIX=aarch64-linux-android
		 	GCC=$TOOLCHAIN/lib/gcc/$TOOLCHAIN_PREFIX/4.9.x/libgcc.a
		 	ARCH=aarch64
		 	CPU=armv8-a
		 	ANDROID_ABI=arm64-v8a
		 	ADDI_CFLAGS=""
		 	ADDI_LDFLAGS=""
		 ;;
		*)
			echo Unknown CPU : $1
			exit
	esac

	PREFIX=$RESULT_DIR/$ANDROID_ABI

	echo ""
	echo "PREFIX : $PREFIX"

	DEFAULT_CFLAGS="-DANDROID -D__STDC_CONSTANT_MACROS -mandroid"
	DEFAULT_CFLAGS="$DEFAULT_CFLAGS -ftree-vectorize -ffunction-sections -funwind-tables -fomit-frame-pointer -funswitch-loops"
	DEFAULT_CFLAGS="$DEFAULT_CFLAGS -finline-limit=300 -finline-functions -fpredictive-commoning -fgcse-after-reload -fipa-cp-clone"
	echo "DEFAULT_CFLAGS : $DEFAULT_CFLAGS"

	echo "DEFAULT_LDFLAGS : $DEFAULT_LDFLAGS"


	echo ""
	echo "pushd"
	pushd $SRC_DIR_NAME

	echo ""
	echo "> configure"
	./configure \
		--prefix=$PREFIX \
		--disable-programs \
		--disable-doc \
		--disable-avdevice \
		--disable-encoders \
		--disable-muxers \
		--disable-devices \
		--disable-filters \
		--enable-openssl \
		--arch=$ARCH \
		--cpu=$CPU \
		--cross-prefix=$TOOLCHAIN/bin/$TOOLCHAIN_PREFIX- \
		--enable-cross-compile \
		--sysroot=$SYSROOT \
		--target-os=linux \
		--extra-cflags="$DEFAULT_CFLAGS $ADDI_CFLAGS" \
		--extra-ldflags="$DEFAULT_LDFLAGS $ADDI_LDFLAGS" \
		--enable-pic \
		--enable-zlib \
		--disable-debug \
		--optflags="-O2"

	echo ""
	echo "> clean"
	make clean

	rm compat/*.o
	rm compat/*.d

	echo ""
	echo "> make"
	make -j3

	echo ""
	echo "> install"
	make install

	echo ""
	echo "> make libffmpeg.so"
	$TOOLCHAIN/bin/$TOOLCHAIN_PREFIX-ld \
		-rpath-link=$SYSROOT/usr/lib \
		-L$SYSROOT/usr/lib \
		-soname libffmpeg.so \
		-shared \
		-nostdlib \
		-z noexecstack \
		-Bsymbolic \
		--whole-archive \
		--no-undefined \
		-o $PREFIX/libffmpeg.so libavcodec/libavcodec.a libavfilter/libavfilter.a libavformat/libavformat.a libavutil/libavutil.a libswscale/libswscale.a libswresample/libswresample.a \
		-lc \
		-lm \
		-lz \
		-ldl \
		-llog \
		--dynamic-linker=/system/bin/linker $GCC

	echo "> copy log"
	cp config.log $PREFIX/config.$1.log

	popd

	echo "======== end build : $1 =========="
}

build armv8-a
build armv7-a
build armv5te
