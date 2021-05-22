#!/bin/bash

./build-lame.sh

./build-ffmpeg.sh arm
./build-ffmpeg.sh arm64
#./build-ffmpeg.sh x86
./build-ffmpeg.sh x86_64

ndk-build NDK_DEBUG=0 \
          NDK_APPLICATION_MK=Application.mk \
				  -e APP_BUILD_SCRIPT=ffmpeg.mk
