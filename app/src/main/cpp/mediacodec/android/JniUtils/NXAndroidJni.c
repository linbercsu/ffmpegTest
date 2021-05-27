/*
 * YXAndroidJni.c
 *
 *  Created on: 2016-11-3
 *      Author:
 */


#include <unistd.h>
#include <pthread.h>
#include "NXAndroidJniBase.h"
#include "NXAndroidOSBuild.h"

#undef YX_LOG_TAG
#define YX_LOG_TAG "YXAndroidJni"


static JavaVM * g_jvm = NULL;

JavaVM *YX_JNI_GetJvm()
{
    return g_jvm;
}

void	YX_JNI_SetJvm(JavaVM * jvm)
{
	g_jvm = jvm;
}


int YX_JNI_AttachThreadEnv(JNIEnv **p_env)
{
	int status = (*g_jvm)->GetEnv(g_jvm, (void**)p_env, JNI_VERSION_1_6);
	if (status < 0)
	{
		if ((*g_jvm)->AttachCurrentThread(g_jvm, p_env, NULL) != JNI_OK)
		{
            YX_ALOGI("%s: AttachCurrentThread() failed", __FUNCTION__);
			return -1;
		}

		return 1;
	}
	return 0;
}

void YX_JNI_DetachThreadEnv()
{
	if ((*g_jvm)->DetachCurrentThread(g_jvm) != JNI_OK)
	{
        YX_ALOGI("%s: DetachCurrentThread() failed", __FUNCTION__);
	}
}

int YX_JNI_ThrowException(JNIEnv* env, const char* className, const char* msg)
{
    if ((*env)->ExceptionCheck(env)) {
        jthrowable exception = (*env)->ExceptionOccurred(env);
        (*env)->ExceptionClear(env);

        if (exception != NULL) {
            YX_ALOGI("Discarding pending exception (%s) to throw", className);
            (*env)->DeleteLocalRef(env, exception);
        }
    }

    jclass exceptionClass = (*env)->FindClass(env, className);
    if (exceptionClass == NULL) {
        YX_ALOGI("Unable to find exception class %s", className);
        /* ClassNotFoundException now pending */
        goto fail;
    }

    if ((*env)->ThrowNew(env, exceptionClass, msg) != JNI_OK) {
        YX_ALOGI("Failed throwing '%s' '%s'", className, msg);
        /* an exception, most likely OOM, will now be pending */
        goto fail;
    }

    return 0;
fail:
    if (exceptionClass)
        (*env)->DeleteLocalRef(env, exceptionClass);
    return -1;
}

int YX_JNI_ThrowIllegalStateException(JNIEnv *env, const char* msg)
{
    return YX_JNI_ThrowException(env, "java/lang/IllegalStateException", msg);
}

void YX_JNI_DeleteGlobalRefP(JNIEnv *env, jobject *obj_ptr)
{
    if (!obj_ptr || !*obj_ptr)
        return;

    (*env)->DeleteGlobalRef(env, *obj_ptr);
    *obj_ptr = NULL;
}

void YX_JNI_DeleteLocalRefP(JNIEnv *env, jobject * obj_ptr)
{
    if (!obj_ptr || !*obj_ptr)
        return;

    (*env)->DeleteLocalRef(env, *obj_ptr);
    *obj_ptr = NULL;
}


jobject YX_JNI_NewObjectAsGlobalRef(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
    va_list args;
    va_start(args, methodID);

    jobject global_object = NULL;
    jobject local_object = (*env)->NewObjectV(env, clazz, methodID, args);
    if (!YX_ExceptionCheck__throwAny(env) && local_object) {
        global_object = (*env)->NewGlobalRef(env, local_object);
        YX_JNI_DeleteLocalRefP(env, &local_object);
    }

    va_end(args);
    return global_object;
}



int YX_Android_GetApiLevel()
{
    static int SDK_INT = 0;
    if (SDK_INT > 0)
        return SDK_INT;

    JNIEnv *env = NULL;
    int status = YX_JNI_AttachThreadEnv( &env);
    if( status<0)
    {
        return -1;
    }


    SDK_INT = YXC_android_os_Build__VERSION__SDK_INT__get__catchAll(env);
    YX_ALOGE("API-Level: %d\n", SDK_INT);


    if( status>0)
    {
        YX_JNI_DetachThreadEnv();
    }


    return SDK_INT;

}
