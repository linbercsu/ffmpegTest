/*
 * YXAndroidJni.h
 *
 *  Created on: 2016-11-3
 *      Author:
 */

#ifndef YXANDROIDJNI_H_
#define YXANDROIDJNI_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "jni.h"
#include "NXAndroidJniBase.h"
#include "NXAndroidAllClass.h"


#define YX_API_1_BASE                      1   // 1.0
#define YX_API_2_BASE_1_1                  2   // 1.1
#define YX_API_3_CUPCAKE                   3   // 1.5
#define YX_API_4_DONUT                     4   // 1.6
#define YX_API_5_ECLAIR                    5   // 2.0
#define YX_API_6_ECLAIR_0_1                6   // 2.0.1
#define YX_API_7_ECLAIR_MR1                7   // 2.1
#define YX_API_8_FROYO                     8   // 2.2
#define YX_API_9_GINGERBREAD               9   // 2.3
#define YX_API_10_GINGERBREAD_MR1          10  // 2.3.3
#define YX_API_11_HONEYCOMB                11  // 3.0
#define YX_API_12_HONEYCOMB_MR1            12  // 3.1
#define YX_API_13_HONEYCOMB_MR2            13  // 3.2
#define YX_API_14_ICE_CREAM_SANDWICH       14  // 4.0
#define YX_API_15_ICE_CREAM_SANDWICH_MR1   15  // 4.0.3
#define YX_API_16_JELLY_BEAN               16  // 4.1
#define YX_API_17_JELLY_BEAN_MR1           17  // 4.2
#define YX_API_18_JELLY_BEAN_MR2           18  // 4.3
#define YX_API_19_KITKAT                   19  // 4.4
#define YX_API_20_KITKAT_WATCH             20  // 4.4W
#define YX_API_21_LOLLIPOP                 21  // 5.0
#define YX_API_22_LOLLIPOP_MR1             22  // 5.1
#define YX_API_23_M                        23  // 6.0

JavaVM *YX_JNI_GetJvm();
void	YX_JNI_SetJvm(JavaVM * jvm);

int 	YX_JNI_AttachThreadEnv( JNIEnv **p_env);
void 	YX_JNI_DetachThreadEnv();

int     YX_JNI_ThrowException(JNIEnv *env, const char *exception, const char* msg);
int     YX_JNI_ThrowIllegalStateException(JNIEnv *env, const char* msg);

jobject YX_JNI_NewObjectAsGlobalRef(JNIEnv *env, jclass clazz, jmethodID methodID, ...);

void    YX_JNI_DeleteGlobalRefP(JNIEnv *env, jobject *obj_ptr);
void    YX_JNI_DeleteLocalRefP(JNIEnv *env, jobject *obj_ptr);

int     YX_Android_GetApiLevel();

#define YX_FIND_JAVA_CLASS(env__, var__, classsign__) \
    do { \
        jclass clazz = (*env__)->FindClass(env__, classsign__); \
        if (J4A_ExceptionCheck__catchAll(env) || !(clazz)) { \
            ALOGE("FindClass failed: %s", classsign__); \
            return -1; \
        } \
        var__ = (*env__)->NewGlobalRef(env__, clazz); \
        if (J4A_ExceptionCheck__catchAll(env) || !(var__)) { \
            ALOGE("FindClass::NewGlobalRef failed: %s", classsign__); \
            (*env__)->DeleteLocalRef(env__, clazz); \
            return -1; \
        } \
        (*env__)->DeleteLocalRef(env__, clazz); \
    } while(0);

#define JNI_CHECK_GOTO(condition__, env__, exception__, msg__, label__) \
    do { \
        if (!(condition__)) { \
            if (exception__) { \
                YX_JNI_ThrowException(env__, exception__, msg__); \
            } \
            goto label__; \
        } \
    }while(0)

#define JNI_CHECK_RET_VOID(condition__, env__, exception__, msg__) \
    do { \
        if (!(condition__)) { \
            if (exception__) { \
                YX_JNI_ThrowException(env__, exception__, msg__); \
            } \
            return; \
        } \
    }while(0)

#define JNI_CHECK_RET(condition__, env__, exception__, msg__, ret__) \
    do { \
        if (!(condition__)) { \
            if (exception__) { \
                YX_JNI_ThrowException(env__, exception__, msg__); \
            } \
            return ret__; \
        } \
    }while(0)

#ifdef __cplusplus
}
#endif



#endif /* YXANDROIDJNI_H_ */
