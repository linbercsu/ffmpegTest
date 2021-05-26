#include <jni.h>
#include <string>
#include <android/log.h>
#include "AudioConverter.h"
#include "mediacodec/NXMediaCodecEncInterface.h"




//extern "C" JNIEXPORT void JNICALL
//Java_com_mx_myapplication_MainActivity_convert(
//        JNIEnv* env,
//        jobject  thzz, jstring source, jstring target, jstring format) {
//    __android_log_write(6, "test", "hello");
//
//    jboolean copy;
//    const char *sourcePath = env->GetStringUTFChars(source, &copy);
//    const char *targetPath = env->GetStringUTFChars(target, &copy);
//    const char *formatUTF = env->GetStringUTFChars(format, &copy);
//
//    auto* converter = new AudioConverter(sourcePath, targetPath, formatUTF);
//    converter->convert();
//    delete converter;
//}

extern "C" JNIEXPORT jint JNICALL
Java_com_mx_myapplication_MainActivity_max(
        JNIEnv* env,
        jobject /* this */) {
    return 0;
}
extern "C" JNIEXPORT jstring JNICALL
Java_com_mx_myapplication_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {

    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv* env = nullptr;
    //判断一下JNI的版本
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        // LOGE("ERROR: GetEnv failed\n");
        return JNI_FALSE;
    }

    YX_AMediaCodec_Enc_loadClassEnv(vm, JNI_VERSION_1_4);

    return JNI_VERSION_1_4;
}


