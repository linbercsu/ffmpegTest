/*
 * YXAndroidOSBuild.c
 *
 *  Created on: 2016-11-3
 *      Author:
 */
#include "NXAndroidOSBuild.h"

typedef struct YXC_android_os_Build__VERSION {
    jclass id;

    jfieldID field_SDK_INT;

} YXC_android_os_Build__VERSION;
static YXC_android_os_Build__VERSION class_YXC_android_os_Build__VERSION;

typedef struct YXC_android_os_Build {
    jclass id;

    jfieldID field_MANUFACTURER;
    jfieldID field_MODEL;
} YXC_android_os_Build;
static YXC_android_os_Build class_YXC_android_os_Build;

jint YXC_android_os_Build__VERSION__SDK_INT__get(JNIEnv *env)
{
    return (*env)->GetStaticIntField(env, class_YXC_android_os_Build__VERSION.id, class_YXC_android_os_Build__VERSION.field_SDK_INT);
}

jint YXC_android_os_Build__VERSION__SDK_INT__get__catchAll(JNIEnv *env)
{
    jint ret_value = YXC_android_os_Build__VERSION__SDK_INT__get(env);
    if (YX_ExceptionCheck__catchAll(env)) {
        return 0;
    }

    return ret_value;
}

void YXC_android_os_Build__VERSION__SDK_INT__set(JNIEnv *env, jint value)
{
    (*env)->SetStaticIntField(env, class_YXC_android_os_Build__VERSION.id, class_YXC_android_os_Build__VERSION.field_SDK_INT, value);
}

void YXC_android_os_Build__VERSION__SDK_INT__set__catchAll(JNIEnv *env, jint value)
{
    YXC_android_os_Build__VERSION__SDK_INT__set(env, value);
    YX_ExceptionCheck__catchAll(env);
}

jstring YXC_android_os_Build__MANUFACTURER__getString(JNIEnv *env)
{
	return (*env)->GetStaticObjectField( env, class_YXC_android_os_Build.id, class_YXC_android_os_Build.field_MANUFACTURER);
}

jstring YXC_android_os_Build__MANUFACTURER__getString__catchAll(JNIEnv *env)
{
	jstring ret_object = YXC_android_os_Build__MANUFACTURER__getString(env);
    if (YX_ExceptionCheck__catchAll(env)|| !ret_object) {
        return NULL;
    }

    return ret_object;
}

const char * YXC_android_os_Build__MANUFACTURER__getString__asCBuffer(JNIEnv *env, char * out_buf, int * out_len)
{
    const char *ret_value = NULL;
    const char *c_str     = NULL;
    jstring local_string = YXC_android_os_Build__MANUFACTURER__getString(env);
    if (YX_ExceptionCheck__throwAny(env) || !local_string) {
        goto fail;
    }

    c_str = (*env)->GetStringUTFChars(env, local_string, NULL );
    if (YX_ExceptionCheck__throwAny(env) || !c_str) {
        goto fail;
    }

    *out_len = strlen(c_str);
    strcpy(out_buf, c_str);
    ret_value = out_buf;

fail:
    YX_ReleaseStringUTFChars__p(env, local_string, &c_str);
    YX_DeleteLocalRef__p(env, &local_string);
    return ret_value;

}

const char * YXC_android_os_Build__MANUFACTURER__getString__asCBuffer__catchAll(JNIEnv *env, char * out_buf, int * out_len)
{
    const char *ret_value = NULL;
    const char *c_str     = NULL;
    jstring local_string = YXC_android_os_Build__MANUFACTURER__getString(env);
    if (YX_ExceptionCheck__throwAny(env) || !local_string) {
        goto fail;
    }

    c_str = (*env)->GetStringUTFChars(env, local_string, NULL );
    if (YX_ExceptionCheck__throwAny(env) || !c_str) {
        goto fail;
    }

    *out_len = strlen(c_str);
    strcpy(out_buf, c_str);
    ret_value = out_buf;

fail:
    YX_ReleaseStringUTFChars__p(env, local_string, &c_str);
    YX_DeleteLocalRef__p(env, &local_string);
    return ret_value;

}


jstring YXC_android_os_Build__MODEL__getString(JNIEnv *env)
{
	return (*env)->GetStaticObjectField( env, class_YXC_android_os_Build.id, class_YXC_android_os_Build.field_MODEL);

}

jstring YXC_android_os_Build__MODEL__getString__catchAll(JNIEnv *env)
{
	jstring ret_object = YXC_android_os_Build__MODEL__getString(env);
    if (YX_ExceptionCheck__catchAll(env)|| !ret_object) {
        return NULL;
    }

    return ret_object;

}

const char * YXC_android_os_Build__MODEL__getString__asCBuffer(JNIEnv *env, char * out_buf, int * out_len)
{
    const char *ret_value = NULL;
    const char *c_str     = NULL;
    jstring local_string = YXC_android_os_Build__MODEL__getString(env);
    if (YX_ExceptionCheck__throwAny(env) || !local_string) {
        goto fail;
    }

    c_str = (*env)->GetStringUTFChars(env, local_string, NULL );
    if (YX_ExceptionCheck__throwAny(env) || !c_str) {
        goto fail;
    }

    *out_len = strlen(c_str);
    strcpy(out_buf, c_str);
    ret_value = out_buf;

fail:
    YX_ReleaseStringUTFChars__p(env, local_string, &c_str);
    YX_DeleteLocalRef__p(env, &local_string);
    return ret_value;

}

const char * YXC_android_os_Build__MODEL__getString__asCBuffer__catchAll(JNIEnv *env, char * out_buf, int * out_len)
{
    const char *ret_value = NULL;
    const char *c_str     = NULL;
    jstring local_string = YXC_android_os_Build__MODEL__getString(env);
    if (YX_ExceptionCheck__throwAny(env) || !local_string) {
        goto fail;
    }

    c_str = (*env)->GetStringUTFChars(env, local_string, NULL );
    if (YX_ExceptionCheck__throwAny(env) || !c_str) {
        goto fail;
    }

    *out_len = strlen(c_str);
    strcpy(out_buf, c_str);
    ret_value = out_buf;

fail:
    YX_ReleaseStringUTFChars__p(env, local_string, &c_str);
    YX_DeleteLocalRef__p(env, &local_string);
    return ret_value;
}


int YX_loadClass__YXC_android_os_Build__VERSION(JNIEnv *env)
{
    int         ret                   = -1;
    const char *YX_UNUSED(name)      = NULL;
    const char *YX_UNUSED(sign)      = NULL;
    jclass      YX_UNUSED(class_id)  = NULL;
    int         YX_UNUSED(api_level) = 0;

    if (class_YXC_android_os_Build__VERSION.id != NULL)
        return 0;

    sign = "android/os/Build$VERSION";
    class_YXC_android_os_Build__VERSION.id = YX_FindClass__asGlobalRef__catchAll(env, sign);
    if (class_YXC_android_os_Build__VERSION.id == NULL)
        goto fail;

    class_id = class_YXC_android_os_Build__VERSION.id;
    name     = "SDK_INT";
    sign     = "I";
    class_YXC_android_os_Build__VERSION.field_SDK_INT = YX_GetStaticFieldID__catchAll(env, class_id, name, sign);
    if (class_YXC_android_os_Build__VERSION.field_SDK_INT == NULL)
        goto fail;

    YX_ALOGD("YXLoader: OK: '%s' loaded\n", "android.os.Build$VERSION");
    ret = 0;
fail:
    return ret;
}




int YX_loadClass__YXC_android_os_Build(JNIEnv *env)
{
    int         ret                   = -1;
    const char *YX_UNUSED(name)      = NULL;
    const char *YX_UNUSED(sign)      = NULL;
    jclass      YX_UNUSED(class_id)  = NULL;
    int         YX_UNUSED(api_level) = 0;

    if (class_YXC_android_os_Build.id != NULL)
        return 0;

    sign = "android/os/Build";
    class_YXC_android_os_Build.id = YX_FindClass__asGlobalRef__catchAll(env, sign);
    if (class_YXC_android_os_Build.id == NULL)
        goto fail;

    ret = YX_loadClass__YXC_android_os_Build__VERSION(env);
    if (ret)
        goto fail;

    YX_ALOGD("YXLoader: OK: '%s' loaded\n", "android.os.Build");
    ret = 0;
fail:
    return ret;
}

