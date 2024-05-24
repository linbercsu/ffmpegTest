//
// Created by linlin zhao on 2024/5/23.
//
#include <jni.h>
#include "Player.h"

jlong createPlayer(JNIEnv *env,
                 jobject thzz, jstring path) {

    jboolean copy;
    const char *sourcePath = env->GetStringUTFChars(path, &copy);

    auto player = new next::Player(std::string(sourcePath));

    env->ReleaseStringUTFChars(path, sourcePath);

    return jlong(player);
}


void nativeOnSurfaceCreated(JNIEnv *env,
                      jobject  /*thzz*/, jlong ptr) {
    auto *player = reinterpret_cast<next::Player *>(ptr);

    player->onSurfaceCreated();

}
void nativeOnDrawFrame(JNIEnv *env,
                      jobject  /*thzz*/, jlong ptr) {
    auto *player = reinterpret_cast<next::Player *>(ptr);

    player->onDrawFrame();

}
void nativeOnSurfaceChanged(JNIEnv *env,
                      jobject  /*thzz*/, jlong ptr, jint w, jint h) {
    auto *player = reinterpret_cast<next::Player *>(ptr);

    player->onSurfaceChanged(w, h);

}
void start(JNIEnv *env,
                      jobject  /*thzz*/, jlong ptr) {
    auto *player = reinterpret_cast<next::Player *>(ptr);

    player->start();

}
void pause(JNIEnv *env,
                      jobject  /*thzz*/, jlong ptr) {
    auto *player = reinterpret_cast<next::Player *>(ptr);

    player->pause();

}

const JNINativeMethod methods[] =
        {
                {"createPlayer",    "(Ljava/lang/String;)J", (void *) createPlayer},
                {"nativeOnSurfaceCreated", "(J)V",                                     (void *) nativeOnSurfaceCreated},
                {"nativeOnSurfaceChanged", "(JII)V",                                     (void *) nativeOnSurfaceChanged},
                {"nativeOnDrawFrame", "(J)V",                                     (void *) nativeOnDrawFrame},
                {"start", "(J)V",                                     (void *) start},
                {"pause",    "(J)V",                                                      (void *) pause}
        };

void initClass(JNIEnv *env, jclass clazz) {
    env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0]));
}

extern "C" JNIEXPORT void JNICALL
Java_com_mxtech_av_TinyPlayer_nativeInitClass(
        JNIEnv* env,
jclass clazz) {
    initClass(env, clazz);
}