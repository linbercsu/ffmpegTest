# Test app for ffmpeg on Android

## Config NDK
Add NDK to your shell Env. somethings like:  
```shell script
export NDK=/Your-Path/Android/sdk/ndk/20.1.5948944
export PATH=${PATH}:${NDK}
```


## Initialize

```shell script
git submodule init
git submodule update

```

## build

`build ffmpeg libs`  
```shell script
cd app/src/main/cpp
./build-all.sh

```
`build app apk`  
```shell script
./gradlew app:assembleDebug
```

## detect memory leak

1. recover '/app/src/main/cpp/ffmpeg/libavutil/mem.c' with '/app/mem.c'  

2. rebuild ffmpeg

3. uncomment '//#define NEXT_DETECT_MEMORY' in JNIPlayer.cpp

4. rebuild app

