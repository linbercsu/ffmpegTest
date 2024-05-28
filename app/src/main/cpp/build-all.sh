#!/bin/bash

function guessAndroidSdk {
  if test -d "$ANDROID_HOME"
  then
    echo "$ANDROID_HOME"
    return 0
    fi
#/Users/lin/Library/Android/sdk
#/home/ubuntu/Android/Sdk
  dirs=$(ls "/Users")
  for one in $dirs
  do
    dir="/Users/${one}/Library/Android/sdk"

    if test -d "${dir}"
    then
        sdk="${dir}"
#      else
#        echo ""
        fi
  done

  if test -z $sdk
  then
      dirs=$(ls "/home")
      for one in $dirs
      do
        dir="/home/${one}/Android/Sdk"

        if test -d "${dir}"
        then
            sdk="${dir}"
    #      else
    #        echo ""
            fi
      done
    fi

  if test -z $sdk
  then
    echo "couldn't guess ANDROID_SDK"
    exit 1
    else
      echo "$sdk"
    fi
}


ANDROID_SDK=$(guessAndroidSdk)

export NDK=${ANDROID_SDK}/ndk/25.2.9519653
#export NDK=${ANDROID_SDK}/ndk/20.1.5948944
export PATH=${PATH}:${NDK}


# clean
rm -rf ffmpeg-build/
rm -rf lame-build/
rm -rf ../jniLibs/
rm -rf ../obj/

# libmp3lame
./build-lame.sh

./build-x264.sh

# ffmpeg
#./build-ffmpeg.sh arm
./build-ffmpeg.sh arm64
#./build-ffmpeg.sh x86
#./build-ffmpeg.sh x86_64



# collect all to ffmpeg-mix
#ndk-build NDK_DEBUG=0 \
#          NDK_APPLICATION_MK=Application.mk \
#				  -e APP_BUILD_SCRIPT=ffmpeg.mk
