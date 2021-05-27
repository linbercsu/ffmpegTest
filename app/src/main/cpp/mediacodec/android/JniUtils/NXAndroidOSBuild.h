/*
 * YXAndroidOSBuild.h
 *
 *  Created on: 2016-11-3
 *      Author:
 */

#ifndef YXANDROIDOSBUILD_H_
#define YXANDROIDOSBUILD_H_
#include "NXAndroidJniBase.h"

jint YXC_android_os_Build__VERSION__SDK_INT__get(JNIEnv *env);
jint YXC_android_os_Build__VERSION__SDK_INT__get__catchAll(JNIEnv *env);
void YXC_android_os_Build__VERSION__SDK_INT__set(JNIEnv *env, jint value);
void YXC_android_os_Build__VERSION__SDK_INT__set__catchAll(JNIEnv *env, jint value);

jstring YXC_android_os_Build__MANUFACTURER__getString(JNIEnv *env);
jstring YXC_android_os_Build__MANUFACTURER__getString__catchAll(JNIEnv *env);
const char * YXC_android_os_Build__MANUFACTURER__getString__asCBuffer(JNIEnv *env, char * out_buf, int * out_len);
const char * YXC_android_os_Build__MANUFACTURER__getString__asCBuffer__catchAll(JNIEnv *env, char * out_buf, int * out_len);

jstring YXC_android_os_Build__MODEL__getString(JNIEnv *env);
jstring YXC_android_os_Build__MODEL__getString__catchAll(JNIEnv *env);
const char * YXC_android_os_Build__MODEL__getString__asCBuffer(JNIEnv *env, char * out_buf, int * out_len);
const char * YXC_android_os_Build__MODEL__getString__asCBuffer__catchAll(JNIEnv *env, char * out_buf, int * out_len);


int YX_loadClass__YXC_android_os_Build(JNIEnv *env);

#endif /* YXANDROIDOSBUILD_H_ */
