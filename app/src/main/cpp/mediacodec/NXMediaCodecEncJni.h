/*
 * com_nxinc_VMediacodec_Enc_jni.h
 *
 *  Created on: 2016-7-27
 *      Author:
 */

#ifndef COM_NXINC_MEDIACODEC_JNI_H_
#define COM_NXINC_MEDIACODEC_JNI_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "android/JniUtils/NxAndroidJniBase.h"


int 		Java_loadClass__com_nxinc_VMediacodec_Enc(JNIEnv *env);

jobject 	com_nxinc_VMediacodec_Enc__createEncoderObject(JNIEnv *env);

jobject 	com_nxinc_VMediacodec_Enc__createEncoderObject__catchAll(JNIEnv *env);

jobject 	com_nxinc_VMediacodec_Enc__createEncoderObject__asGlobalRef__catchAll(JNIEnv *env);

jboolean 	com_nxinc_VMediacodec_Enc__isInNotSupportedList(JNIEnv *env);

jint 		com_nxinc_VMediacodec_Enc__initEncoder( JNIEnv *env, jobject thiz, jint width, jint height, jint frameRate, jint colorFormat, jint iFrameInterval, jint bitRate, jint profile, jboolean useInputSurface, int _encType);

jint 		com_nxinc_VMediacodec_Enc__encodeVideoFromBuffer( JNIEnv *env, jobject thiz, jbyteArray input, jbyteArray output, int64_t pts);

jint 		com_nxinc_VMediacodec_Enc__encodeVideoFromBufferAsyn( JNIEnv *env, jobject thiz, jbyteArray input, jbyteArray output);

jint 		com_nxinc_VMediacodec_Enc__encodeVideoFromTexture( JNIEnv *env, jobject thiz, jintArray input, jbyteArray output);

jint 		com_nxinc_VMediacodec_Enc__encodeVideoFromTextureAsyn( JNIEnv *env, jobject thiz, jintArray input, jbyteArray output);

jobject 	com_nxinc_VMediacodec_Enc__getInputSurface( JNIEnv *env, jobject thiz);

jlong		com_nxinc_VMediacodec_Enc__getLastPts( JNIEnv * env, jobject thiz);

jint        com_nxinc_VMediacodec_Enc__getLastFramFlags( JNIEnv *env, jobject thiz);

jint 		com_nxinc_VMediacodec_Enc__closeEncoder( JNIEnv * env, jobject thiz);

jint 		com_nxinc_VMediacodec_Enc__closeEncoderAsyn( JNIEnv * env, jobject thiz);

jint 		com_nxinc_VMediacodec_Enc__getSupportedColorFormat( JNIEnv * env, jobject thiz);

jint 		com_nxinc_VMediacodec_Enc__getExtraData( JNIEnv * env, jobject thiz, jbyteArray output);

jint		com_nxinc_VMediacodec_Enc__setEncoder( JNIEnv *env, jobject thiz, jint width, jint height, jint frameRate, jint bitRate, jint iFrameInterval, jint colorFormat, jint profile);

jint		com_nxinc_VMediacodec_Enc__getInfoByFlag( JNIEnv * env, jobject thiz, jintArray output, int flag);

#ifdef __cplusplus
}
#endif


#endif /* COM_NXINC_MEDIACODEC_JNI_H_ */
