/*
 * YXAndroidJniBase.h
 *
 *  Created on: 2016-11-3
 *      Author:
 */

#ifndef YXANDROIDJNIBASE_H_
#define YXANDROIDJNIBASE_H_


#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <jni.h>
#include <android/log.h>

#ifndef YX_UNUSED
#define YX_UNUSED(x) x __attribute__((unused))
#endif

#define YX_LOG_TAG "YX"
#define YX_VLOGV(...)  __android_log_vprint(ANDROID_LOG_VERBOSE,   YX_LOG_TAG, __VA_ARGS__)
#define YX_VLOGD(...)  __android_log_vprint(ANDROID_LOG_DEBUG,     YX_LOG_TAG, __VA_ARGS__)
#define YX_VLOGI(...)  __android_log_vprint(ANDROID_LOG_INFO,      YX_LOG_TAG, __VA_ARGS__)
#define YX_VLOGW(...)  __android_log_vprint(ANDROID_LOG_WARN,      YX_LOG_TAG, __VA_ARGS__)
#define YX_VLOGE(...)  __android_log_vprint(ANDROID_LOG_ERROR,     YX_LOG_TAG, __VA_ARGS__)

#define YX_ALOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE,    YX_LOG_TAG, __VA_ARGS__)
#define YX_ALOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,      YX_LOG_TAG, __VA_ARGS__)
#define YX_ALOGI(...)  __android_log_print(ANDROID_LOG_INFO,       YX_LOG_TAG, __VA_ARGS__)
#define YX_ALOGW(...)  __android_log_print(ANDROID_LOG_WARN,       YX_LOG_TAG, __VA_ARGS__)
#define YX_ALOGE(...)  __android_log_print(ANDROID_LOG_ERROR,      YX_LOG_TAG, __VA_ARGS__)


#ifndef YXLOGI
#define YXLOGI YX_ALOGI
#endif

#ifndef YXLOGD
#define YXLOGD YX_ALOGD
#endif

#ifndef YXLOGE
#define YXLOGE YX_ALOGE
#endif


#define YX_FUNC_FAIL_TRACE()               do {YX_ALOGE("%s: failed\n", __func__);} while (0)
#define YX_FUNC_FAIL_TRACE1(x__)           do {YX_ALOGE("%s: failed: %s\n", __func__, x__);} while (0)
#define YX_FUNC_FAIL_TRACE2(x1__, x2__)    do {YX_ALOGE("%s: failed: %s %s\n", __func__, x1__, x2__);} while (0)

#define YX_LOAD_CLASS(class__) \
    do { \
        ret = YX_loadClass__YXC_##class__(env); \
        if (ret) \
            goto fail; \
    } while (0)

/********************
 * Exception Handle
 ********************/

bool YX_ExceptionCheck__throwAny(JNIEnv *env);
bool YX_ExceptionCheck__catchAll(JNIEnv *env);
int  YX_ThrowExceptionOfClass(JNIEnv* env, jclass clazz, const char* msg);
int  YX_ThrowException(JNIEnv* env, const char* class_sign, const char* msg);
int  YX_ThrowIllegalStateException(JNIEnv *env, const char* msg);

/********************
 * References
 ********************/
jclass YX_NewGlobalRef__catchAll(JNIEnv *env, jobject obj);
void   YX_DeleteLocalRef(JNIEnv *env, jobject obj);
void   YX_DeleteLocalRef__p(JNIEnv *env, jobject *obj);
void   YX_DeleteGlobalRef(JNIEnv *env, jobject obj);
void   YX_DeleteGlobalRef__p(JNIEnv *env, jobject *obj);

void   YX_ReleaseStringUTFChars(JNIEnv *env, jstring str, const char *c_str);
void   YX_ReleaseStringUTFChars__p(JNIEnv *env, jstring str, const char **c_str);

/********************
 * Class Load
 ********************/

int    YX_LoadAll__catchAll(JNIEnv *env);
jclass YX_FindClass__catchAll(JNIEnv *env, const char *class_sign);
jclass YX_FindClass__asGlobalRef__catchAll(JNIEnv *env, const char *class_sign);

jmethodID YX_GetMethodID__catchAll(JNIEnv *env, jclass clazz, const char *method_name, const char *method_sign);
jmethodID YX_GetStaticMethodID__catchAll(JNIEnv *env, jclass clazz, const char *method_name, const char *method_sign);

jfieldID YX_GetFieldID__catchAll(JNIEnv *env, jclass clazz, const char *field_name, const char *method_sign);
jfieldID YX_GetStaticFieldID__catchAll(JNIEnv *env, jclass clazz, const char *field_name, const char *method_sign);

/********************
 * Misc Functions
 ********************/

jbyteArray YX_NewByteArray__catchAll(JNIEnv *env, jsize capacity);
jbyteArray YX_NewByteArray__asGlobalRef__catchAll(JNIEnv *env, jsize capacity);

int YX_GetSystemAndroidApiLevel(JNIEnv *env);


#endif /* YXANDROIDJNIBASE_H_ */
