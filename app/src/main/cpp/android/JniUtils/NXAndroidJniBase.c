/*
 * YXAndroidJniBase.c
 *
 *  Created on: 2016-11-3
 *      Author:
 */


#include "NXAndroidJniBase.h"
#include "NXAndroidOSBuild.h"

/********************
 * Exception Handle
 ********************/

bool YX_ExceptionCheck__throwAny(JNIEnv *env)
{
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        return true;
    }

    return false;
}

bool YX_ExceptionCheck__catchAll(JNIEnv *env)
{
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        return true;
    }

    return false;
}

int YX_ThrowExceptionOfClass(JNIEnv* env, jclass clazz, const char* msg)
{
    if ((*env)->ThrowNew(env, clazz, msg) != JNI_OK)
        YX_ALOGE("%s: Failed: msg: '%s'\n", __func__, msg);

    return 0;
}

int YX_ThrowException(JNIEnv* env, const char* class_sign, const char* msg)
{
    int ret = -1;

    if (YX_ExceptionCheck__catchAll(env)) {
        YX_ALOGE("pending exception throwed.\n");
    }

    jclass exceptionClass = YX_FindClass__catchAll(env, class_sign);
    if (exceptionClass == NULL) {
        YX_FUNC_FAIL_TRACE();
        ret = -1;
        goto fail;
    }

    ret = YX_ThrowExceptionOfClass(env, exceptionClass, msg);
    if (ret) {
        YX_FUNC_FAIL_TRACE();
        goto fail;
    }

    ret = 0;
fail:
    YX_DeleteLocalRef__p(env, exceptionClass);
    return ret;
}

int YX_ThrowIllegalStateException(JNIEnv *env, const char* msg)
{
    return YX_ThrowException(env, "java/lang/IllegalStateException", msg);
}

/********************
 * References
 ********************/

jclass YX_NewGlobalRef__catchAll(JNIEnv *env, jobject obj)
{
    jclass obj_global = (*env)->NewGlobalRef(env, obj);
    if (YX_ExceptionCheck__catchAll(env) || !(obj_global)) {
        YX_FUNC_FAIL_TRACE();
        goto fail;
    }

fail:
    return obj_global;
}

void YX_DeleteLocalRef(JNIEnv *env, jobject obj)
{
    if (!obj)
        return;
    (*env)->DeleteLocalRef(env, obj);
}

void YX_DeleteLocalRef__p(JNIEnv *env, jobject *obj)
{
    if (!obj)
        return;
    YX_DeleteLocalRef(env, *obj);
    *obj = NULL;
}

void YX_DeleteGlobalRef(JNIEnv *env, jobject obj)
{
    if (!obj)
        return;
    (*env)->DeleteGlobalRef(env, obj);
}

void YX_DeleteGlobalRef__p(JNIEnv *env, jobject *obj)
{
    if (!obj)
        return;
    YX_DeleteGlobalRef(env, *obj);
    *obj = NULL;
}

void YX_ReleaseStringUTFChars(JNIEnv *env, jstring str, const char *c_str)
{
    if (!str || !c_str)
        return;
    (*env)->ReleaseStringUTFChars(env, str, c_str);
}

void YX_ReleaseStringUTFChars__p(JNIEnv *env, jstring str, const char **c_str)
{
    if (!str || !c_str)
        return;
    YX_ReleaseStringUTFChars(env, str, *c_str);
    *c_str = NULL;
}

/********************
 * Class Load
 ********************/

jclass YX_FindClass__catchAll(JNIEnv *env, const char *class_sign)
{
    jclass clazz = (*env)->FindClass(env, class_sign);
    if (YX_ExceptionCheck__catchAll(env) || !(clazz)) {
        YX_FUNC_FAIL_TRACE();
        clazz = NULL;
        goto fail;
    }

fail:
    return clazz;
}

jclass YX_FindClass__asGlobalRef__catchAll(JNIEnv *env, const char *class_sign)
{
    jclass clazz_global = NULL;
    jclass clazz = YX_FindClass__catchAll(env, class_sign);
    if (!clazz) {
        YX_FUNC_FAIL_TRACE1(class_sign);
        goto fail;
    }

    clazz_global = YX_NewGlobalRef__catchAll(env, clazz);
    if (!clazz_global) {
        YX_FUNC_FAIL_TRACE1(class_sign);
        goto fail;
    }

fail:
    YX_DeleteLocalRef__p(env, &clazz);
    return clazz_global;
}

jmethodID YX_GetMethodID__catchAll(JNIEnv *env, jclass clazz, const char *method_name, const char *method_sign)
{
    jmethodID method_id = (*env)->GetMethodID(env, clazz, method_name, method_sign);
    if (YX_ExceptionCheck__catchAll(env) || !method_id) {
        YX_FUNC_FAIL_TRACE2(method_name, method_sign);
        method_id = NULL;
        goto fail;
    }

fail:
    return method_id;
}

jmethodID YX_GetStaticMethodID__catchAll(JNIEnv *env, jclass clazz, const char *method_name, const char *method_sign)
{
    jmethodID method_id = (*env)->GetStaticMethodID(env, clazz, method_name, method_sign);
    if (YX_ExceptionCheck__catchAll(env) || !method_id) {
        YX_FUNC_FAIL_TRACE2(method_name, method_sign);
        method_id = NULL;
        goto fail;
    }

fail:
    return method_id;
}

jfieldID YX_GetFieldID__catchAll(JNIEnv *env, jclass clazz, const char *field_name, const char *field_sign)
{
    jfieldID field_id = (*env)->GetFieldID(env, clazz, field_name, field_sign);
    if (YX_ExceptionCheck__catchAll(env) || !field_id) {
        YX_FUNC_FAIL_TRACE2(field_name, field_sign);
        field_id = NULL;
        goto fail;
    }

fail:
    return field_id;
}

jfieldID YX_GetStaticFieldID__catchAll(JNIEnv *env, jclass clazz, const char *field_name, const char *field_sign)
{
    jfieldID field_id = (*env)->GetStaticFieldID(env, clazz, field_name, field_sign);
    if (YX_ExceptionCheck__catchAll(env) || !field_id) {
        YX_FUNC_FAIL_TRACE2(field_name, field_sign);
        field_id = NULL;
        goto fail;
    }

fail:
    return field_id;
}

/********************
 * Misc Functions
 ********************/

jbyteArray YX_NewByteArray__catchAll(JNIEnv *env, jsize capacity)
{
    jbyteArray local = (*env)->NewByteArray(env, capacity);
    if (YX_ExceptionCheck__catchAll(env) || !local)
        return NULL;

    return local;
}

jbyteArray YX_NewByteArray__asGlobalRef__catchAll(JNIEnv *env, jsize capacity)
{
    jbyteArray local = (*env)->NewByteArray(env, capacity);
    if (YX_ExceptionCheck__catchAll(env) || !local)
        return NULL;

    jbyteArray global = (*env)->NewGlobalRef(env, local);
    YX_DeleteLocalRef__p(env, &local);
    return global;
}

int YX_GetSystemAndroidApiLevel(JNIEnv *env)
{
    static int SDK_INT = 0;
    if (SDK_INT > 0)
        return SDK_INT;

    SDK_INT = YXC_android_os_Build__VERSION__SDK_INT__get__catchAll(env);
    YX_ALOGI("API-Level: %d\n", SDK_INT);
    return SDK_INT;
}
