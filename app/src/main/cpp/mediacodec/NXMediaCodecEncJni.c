/*
 * com_nxinc_VMediacodec_Enc_jni.c
 *
 *  Created on: 2016-7-27
 *      Author:
 */

#include "NXMediaCodecEncJni.h"


typedef struct com_nxinc_VMediacodec_Enc
{
    jclass id;

    jmethodID method_createEncoderObject;
    jmethodID method_isInNotSupportedList;
    jmethodID method_initEncoder;
	jmethodID method_encodeVideoFromBuffer;
	jmethodID method_encodeVideoFromBufferAsyn;
    jmethodID method_encodeVideoFromTexture;
	jmethodID method_encodeVideoFromTextureAsyn;
	jmethodID method_getInputSurface;
	jmethodID method_getLastFrameFlags;
    jmethodID method_closeEncoder;
	jmethodID method_closeEncoderAsyn;
    jmethodID method_getSurpportedColorFormat;
    jmethodID method_getExtraData;
    jmethodID method_setEncoder;
    jmethodID method_getInfoByFlag;
    jmethodID method_getLastPts;
}com_nxinc_VMediacodec_Enc;

static com_nxinc_VMediacodec_Enc class_com_nxinc_VMediacodec_Enc;

jobject 	com_nxinc_VMediacodec_Enc__createEncoderObject(JNIEnv *env)
{
	return (*env)->CallStaticObjectMethod( env, class_com_nxinc_VMediacodec_Enc.id, class_com_nxinc_VMediacodec_Enc.method_createEncoderObject);
}

jobject 	com_nxinc_VMediacodec_Enc__createEncoderObject__catchAll(JNIEnv *env)
{
    jobject ret_object = com_nxinc_VMediacodec_Enc__createEncoderObject(env);
    if (YX_ExceptionCheck__catchAll(env) || !ret_object) {
        return NULL;
    }

    return ret_object;

}

jobject 	com_nxinc_VMediacodec_Enc__createEncoderObject__asGlobalRef__catchAll(JNIEnv *env)
{
    jobject ret_object   = NULL;
    jobject local_object = com_nxinc_VMediacodec_Enc__createEncoderObject__catchAll(env);
    if (YX_ExceptionCheck__catchAll(env) || !local_object) {
        ret_object = NULL;
        goto fail;
    }

    ret_object = YX_NewGlobalRef__catchAll(env, local_object);
    if (!ret_object) {
        ret_object = NULL;
        goto fail;
    }

fail:
    YX_DeleteLocalRef__p(env, &local_object);
    return ret_object;
}

jboolean 	com_nxinc_VMediacodec_Enc__isInNotSupportedList(JNIEnv *env)
{
	return (*env)->CallStaticBooleanMethod( env, class_com_nxinc_VMediacodec_Enc.id, class_com_nxinc_VMediacodec_Enc.method_isInNotSupportedList);
}

jint 		com_nxinc_VMediacodec_Enc__initEncoder( JNIEnv *env, jobject thiz, jint width, jint height, jint frameRate, jint colorFormat, jint iFrameInterval, jint bitRate, jint profile, jboolean useInputSurface, int _encType)
{
	return (*env)->CallIntMethod( env, thiz, class_com_nxinc_VMediacodec_Enc.method_initEncoder, width, height, frameRate, colorFormat, iFrameInterval, bitRate, profile, useInputSurface, _encType);
}

jint 		com_nxinc_VMediacodec_Enc__encodeVideoFromBuffer( JNIEnv *env, jobject thiz, jbyteArray input, jbyteArray output, int64_t pts)
{
	return (*env)->CallIntMethod( env, thiz, class_com_nxinc_VMediacodec_Enc.method_encodeVideoFromBuffer, input, output, pts);

}

jint 		com_nxinc_VMediacodec_Enc__encodeVideoFromBufferAsyn( JNIEnv *env, jobject thiz, jbyteArray input, jbyteArray output)
{
	return (*env)->CallIntMethod( env, thiz, class_com_nxinc_VMediacodec_Enc.method_encodeVideoFromBufferAsyn, input, output);
}


jint 		com_nxinc_VMediacodec_Enc__encodeVideoFromTexture( JNIEnv *env, jobject thiz, jintArray input, jbyteArray output)
{
	return (*env)->CallIntMethod( env, thiz, class_com_nxinc_VMediacodec_Enc.method_encodeVideoFromTexture, input, output);

}

jint 		com_nxinc_VMediacodec_Enc__encodeVideoFromTextureAsyn( JNIEnv *env, jobject thiz, jintArray input, jbyteArray output)
{
	return (*env)->CallIntMethod( env, thiz, class_com_nxinc_VMediacodec_Enc.method_encodeVideoFromTextureAsyn, input, output);
}

jobject 	com_nxinc_VMediacodec_Enc__getInputSurface( JNIEnv *env, jobject thiz)
{
	return (*env)->CallObjectMethod( env, thiz, class_com_nxinc_VMediacodec_Enc.method_getInputSurface);
}


int        com_nxinc_VMediacodec_Enc__getLastFramFlags( JNIEnv *env, jobject thiz)
{
	return (*env)->CallIntMethod( env, thiz, class_com_nxinc_VMediacodec_Enc.method_getLastFrameFlags);
}


jint 		com_nxinc_VMediacodec_Enc__closeEncoder( JNIEnv * env, jobject thiz)
{
	return (*env)->CallIntMethod( env, thiz, class_com_nxinc_VMediacodec_Enc.method_closeEncoder);
}

jint 		com_nxinc_VMediacodec_Enc__closeEncoderAsyn( JNIEnv * env, jobject thiz)
{
	return (*env)->CallIntMethod( env, thiz, class_com_nxinc_VMediacodec_Enc.method_closeEncoderAsyn);
}

jint 		com_nxinc_VMediacodec_Enc__getSupportedColorFormat( JNIEnv * env, jobject thiz)
{
	return (*env)->CallIntMethod( env, thiz, class_com_nxinc_VMediacodec_Enc.method_getSurpportedColorFormat);
}

jint 		com_nxinc_VMediacodec_Enc__getExtraData( JNIEnv * env, jobject thiz, jbyteArray output)
{
	return (*env)->CallIntMethod( env, thiz, class_com_nxinc_VMediacodec_Enc.method_getExtraData, output);
}

jint		com_nxinc_VMediacodec_Enc__setEncoder( JNIEnv *env, jobject thiz, jint width, jint height, jint frameRate, jint bitRate, jint iFrameInterval, jint profile, jint colorFormat)
{
	return (*env)->CallIntMethod( env, thiz, class_com_nxinc_VMediacodec_Enc.method_setEncoder, width, height, frameRate, bitRate, iFrameInterval, colorFormat, profile);
}

jint		com_nxinc_VMediacodec_Enc__getInfoByFlag( JNIEnv * env, jobject thiz, jintArray output, int flag)
{
	return (*env)->CallIntMethod( env, thiz, class_com_nxinc_VMediacodec_Enc.method_getInfoByFlag, output, flag);
}


jlong		com_nxinc_VMediacodec_Enc__getLastPts( JNIEnv * env, jobject thiz)
{
	return (*env)->CallLongMethod( env, thiz, class_com_nxinc_VMediacodec_Enc.method_getLastPts);
}



int 	Java_loadClass__com_nxinc_VMediacodec_Enc(JNIEnv *env)
{
	int ret = -1;
	const char *YX_UNUSED(name) = NULL;
	const char *YX_UNUSED(sign) = NULL;
	jclass YX_UNUSED(class_id) = NULL;
	int YX_UNUSED(api_level) = 0;

	if (class_com_nxinc_VMediacodec_Enc.id != NULL)
		return 0;

	api_level = YX_GetSystemAndroidApiLevel(env);

	if (api_level < 16) {
		YX_ALOGW(
				"YXLoader: Ignore: '%s' need API %d\n", "android.media.MediaCodec", api_level);
		goto ignore;
	}

	sign = "com/mxtech/av/NXAvcEncoder";
	class_com_nxinc_VMediacodec_Enc.id =
			YX_FindClass__asGlobalRef__catchAll(env, sign);
	if (class_com_nxinc_VMediacodec_Enc.id == NULL)
		goto fail;

	class_id = class_com_nxinc_VMediacodec_Enc.id;
	name = "isInNotSupportedList";
	sign = "()Z";
	class_com_nxinc_VMediacodec_Enc.method_isInNotSupportedList =
			YX_GetStaticMethodID__catchAll(env, class_id, name, sign);
	if (class_com_nxinc_VMediacodec_Enc.method_isInNotSupportedList == NULL)
		goto fail;

	class_id = class_com_nxinc_VMediacodec_Enc.id;
	name = "createEncoderObject";
	sign = "()Lcom/mxtech/av/NXAvcEncoder;";
	class_com_nxinc_VMediacodec_Enc.method_createEncoderObject =
			YX_GetStaticMethodID__catchAll(env, class_id, name, sign);
	if (class_com_nxinc_VMediacodec_Enc.method_createEncoderObject == NULL) {
		YX_ALOGE("load createEncoderObject failed!!!");
		goto fail;
	} else {
		YX_ALOGE("load createEncoderObject success!!!");

	}

	class_id = class_com_nxinc_VMediacodec_Enc.id;
	name = "initEncoder";
	sign = "(IIIIIIIZI)I";
	class_com_nxinc_VMediacodec_Enc.method_initEncoder =
			YX_GetMethodID__catchAll(env, class_id, name, sign);
	if (class_com_nxinc_VMediacodec_Enc.method_initEncoder == NULL)
		goto fail;


	class_id = class_com_nxinc_VMediacodec_Enc.id;
	name = "encodeVideoFromBuffer";
	sign = "([B[BJ)I";
	class_com_nxinc_VMediacodec_Enc.method_encodeVideoFromBuffer =
			YX_GetMethodID__catchAll(env, class_id, name, sign);
	if (class_com_nxinc_VMediacodec_Enc.method_encodeVideoFromBuffer == NULL)
		goto fail;

	class_id = class_com_nxinc_VMediacodec_Enc.id;
	name = "encodeVideoFromBufferAsyn";
	sign = "([B[B)I";
	class_com_nxinc_VMediacodec_Enc.method_encodeVideoFromBufferAsyn =
			YX_GetMethodID__catchAll(env, class_id, name, sign);
	if (class_com_nxinc_VMediacodec_Enc.method_encodeVideoFromBufferAsyn == NULL)
		goto fail;

	class_id = class_com_nxinc_VMediacodec_Enc.id;
	name = "encodeVideoFromTexture";
	sign = "([I[B)I";
	class_com_nxinc_VMediacodec_Enc.method_encodeVideoFromTexture =
			YX_GetMethodID__catchAll(env, class_id, name, sign);
	if (class_com_nxinc_VMediacodec_Enc.method_encodeVideoFromTexture == NULL)
		goto fail;

	class_id = class_com_nxinc_VMediacodec_Enc.id;
	name = "encodeVideoFromTextureAsyn";
	sign = "([I[B)I";
	class_com_nxinc_VMediacodec_Enc.method_encodeVideoFromTextureAsyn =
			YX_GetMethodID__catchAll(env, class_id, name, sign);
	if (class_com_nxinc_VMediacodec_Enc.method_encodeVideoFromTextureAsyn == NULL)
		goto fail;

	class_id = class_com_nxinc_VMediacodec_Enc.id;
	name = "getInputSurface";
	sign = "()Landroid/view/Surface;";
	class_com_nxinc_VMediacodec_Enc.method_getInputSurface =
			YX_GetMethodID__catchAll(env, class_id, name, sign);
	if (class_com_nxinc_VMediacodec_Enc.method_getInputSurface == NULL)
		goto fail;

	class_id = class_com_nxinc_VMediacodec_Enc.id;
	name = "getLastFrameFlags";
	sign = "()I";
	class_com_nxinc_VMediacodec_Enc.method_getLastFrameFlags =
			YX_GetMethodID__catchAll(env, class_id, name, sign);
	if (class_com_nxinc_VMediacodec_Enc.method_getLastFrameFlags == NULL)
		goto fail;


	class_id = class_com_nxinc_VMediacodec_Enc.id;
	name = "closeEncoder";
	sign = "()I";
	class_com_nxinc_VMediacodec_Enc.method_closeEncoder =
			YX_GetMethodID__catchAll(env, class_id, name, sign);
	if (class_com_nxinc_VMediacodec_Enc.method_closeEncoder == NULL)
		goto fail;

	class_id = class_com_nxinc_VMediacodec_Enc.id;
	name = "closeEncoderAsyn";
	sign = "()I";
	class_com_nxinc_VMediacodec_Enc.method_closeEncoderAsyn =
			YX_GetMethodID__catchAll(env, class_id, name, sign);
	if (class_com_nxinc_VMediacodec_Enc.method_closeEncoderAsyn == NULL)
		goto fail;

	class_id = class_com_nxinc_VMediacodec_Enc.id;
	name = "getSupportedColorFormat";
	sign = "()I";
	class_com_nxinc_VMediacodec_Enc.method_getSurpportedColorFormat =
			YX_GetMethodID__catchAll(env, class_id, name, sign);
	if (class_com_nxinc_VMediacodec_Enc.method_getSurpportedColorFormat == NULL)
		goto fail;

	class_id = class_com_nxinc_VMediacodec_Enc.id;
	name = "getExtraData";
	sign = "([B)I";
	class_com_nxinc_VMediacodec_Enc.method_getExtraData =
			YX_GetMethodID__catchAll(env, class_id, name, sign);
	if (class_com_nxinc_VMediacodec_Enc.method_getExtraData == NULL)
		goto fail;

	class_id = class_com_nxinc_VMediacodec_Enc.id;
	name = "setEncoder";
	sign = "(IIIIIII)I";
	class_com_nxinc_VMediacodec_Enc.method_setEncoder =
			YX_GetMethodID__catchAll(env, class_id, name, sign);
	if (class_com_nxinc_VMediacodec_Enc.method_setEncoder == NULL)
		goto fail;

	class_id = class_com_nxinc_VMediacodec_Enc.id;
	name = "getInfoByFlag";
	sign = "([II)I";
	class_com_nxinc_VMediacodec_Enc.method_getInfoByFlag =
			YX_GetMethodID__catchAll(env, class_id, name, sign);
	if (class_com_nxinc_VMediacodec_Enc.method_getInfoByFlag == NULL)
		goto fail;

    class_id = class_com_nxinc_VMediacodec_Enc.id;
	name = "getLastPts";
	sign = "()J";
	class_com_nxinc_VMediacodec_Enc.method_getLastPts =
			YX_GetMethodID__catchAll(env, class_id, name, sign);
	if (class_com_nxinc_VMediacodec_Enc.method_getLastPts == NULL)
		goto fail;


	YX_ALOGE("Encoder Loader: OK: '%s' loaded\n", "NXAvcEncoder");
ignore:
	ret = 0;
fail:
	return ret;
}
