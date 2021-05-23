# Test app for ffmpeg on Android

## Config NDK
Add NDK to your shell Env. somethings like:  
```shell script
export NDK=/Your-Path/Android/sdk/ndk/20.1.5948944
```


## Initialize

```shell script
git submodule init
git submodule update

```

## build

`build ffmpeg libs`  
```shell script
cd src/main/cpp
./build-all

```
`build app apk`  
```shell script
./gradlew app:assembleDebug
```