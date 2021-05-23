# Test app for ffmpeg on Android

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