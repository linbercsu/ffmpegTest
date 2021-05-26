/*
 * com_nxinc_VMediacodec_Encinterface.c
 *
 *  Created on: 2016-7-27
 *      Author:
 */

#include <malloc.h>
#include <jni.h>
#include "NXMediaCodecEncInterface.h"
#include "NXMediaCodecEncJni.h"
#include "android/JniUtils/NXAndroidJni.h"
#include "NXUtilCodecInfoProcess.h"

#undef LOG_TAG
#define LOG_TAG "YXMediaCodecInterface"

#define H264_NALU_TYPE_NON_IDR_PICTURE								1
#define H264_NALU_TYPE_IDR_PICTURE									5
#define H264_NALU_TYPE_SEQUENCE_PARAMETER_SET						7
#define H264_NALU_TYPE_PICTURE_PARAMETER_SET						8
#define H264_NALU_TYPE_SEI                                          6
#define USR_DEF_MAX_PACKET_SIZE 1080*1920*4

struct YX_AMediaCodec_Enc_Opaque
{
	JavaVM * jvm;
	jobject	 obj;
	jobject  inputSurface;
	jbyteArray inputBuffer;
	jintArray inputBufferExt;
	jbyteArray outputBuffer;

	bool isNeedRefresh;
	bool isUseAsyn;

	// sps and pps data
	uint8_t* headerData;
	int headerSize;
	int	headerBufSize;

	int colorFormat;
	int enc_type;

};


int		YX_AMediaCodec_Enc_loadClassEnv( JavaVM * _pGloabJvm, int _jniVersion)
{
	JavaVM * jvm = _pGloabJvm;
	JNIEnv *env = NULL;
	int status = (*jvm)->GetEnv(jvm, (void**)&env, _jniVersion);
	YX_JNI_SetJvm(jvm);
	jint result = YX_LoadAll__catchAll(env);
	JNI_CHECK_RET(result == 0, env, NULL, NULL, -1);
    Java_loadClass__com_nxinc_VMediacodec_Enc(env);
	return 0;
}



bool 	YX_AMediaCodec_Enc_isInNotSupportedList( )
{
	JNIEnv * env = NULL;
	bool bRet = false;
	YXLOGI( "Into YX_AMediaCodec_Enc_isInNotSupportedList!!!");
	int status = YX_JNI_AttachThreadEnv(&env);
	if ( status<0)
	{
		return  false;
	}

	bRet = com_nxinc_VMediacodec_Enc__isInNotSupportedList( env);

	if( status>0)
	{
		YX_JNI_DetachThreadEnv();
	}

	return bRet;

}

YX_AMediaCodec_Enc * YX_AMediaCodec_Enc_createEncoderObject()
{
	YX_AMediaCodec_Enc * ctx = (YX_AMediaCodec_Enc*)malloc(sizeof(YX_AMediaCodec_Enc));
	YX_AMediaCodec_Enc_Opaque * opaque = (YX_AMediaCodec_Enc_Opaque*)malloc(sizeof(YX_AMediaCodec_Enc_Opaque));
	JNIEnv * env = NULL;
	YXLOGI( "Into YX_AMediaCodec_Enc_createEncoderObject!!!");

	if(NULL==opaque||NULL==ctx)
	{
		goto fail;
	}

	int status = YX_JNI_AttachThreadEnv(&env);
	if ( status<0)
	{
		goto fail;
	}

	memset( ctx, 0, sizeof(YX_AMediaCodec_Enc));
	memset( opaque, 0, sizeof(YX_AMediaCodec_Enc_Opaque));
	ctx->opaque = opaque;
	opaque->jvm = YX_JNI_GetJvm();
	opaque->obj = com_nxinc_VMediacodec_Enc__createEncoderObject__asGlobalRef__catchAll( env);
	if( NULL==opaque->obj)
	{
		YXLOGI( "Java_encoder__createEncoderObject__asGlobalRef__catchAll failed!!!");
		goto fail;
	}
	opaque->headerData = (uint8_t*)malloc(1024);
	opaque->headerBufSize = 1024;
	opaque->headerSize = 0;


	YXLOGI( "YX_AMediaCodec_Enc_createEncoderObject ctx:[%p] opaque:[%p] obj:[%p]!!!", ctx, opaque, opaque->obj);


	if( status>0)
	{
		YX_JNI_DetachThreadEnv();
	}
	return ctx;

fail:
	if( NULL != ctx)
	{
		free(ctx);
		ctx=NULL;
	}
	if( NULL != opaque)
	{
		free(opaque);
		opaque = NULL;
	}

	if( status>0)
	{
		YX_JNI_DetachThreadEnv();
	}
	return NULL;
}

void 	YX_AMediaCodec_Enc_destoryEncoderObject( YX_AMediaCodec_Enc * _ctx)
{
	YX_AMediaCodec_Enc_Opaque * opaque = _ctx->opaque;
	JNIEnv * env = NULL;
	int status = YX_JNI_AttachThreadEnv(&env);
	if( status < 0)
	{
		YXLOGI( "YX_JNI_SetupThreadEnv failed!!!");
		return;
	}

	if( NULL != opaque->headerData)
	{
		free(opaque->headerData);
		opaque->headerData = NULL;
		opaque->headerBufSize = 0;
		opaque->headerSize = 0;
	}

	YX_DeleteGlobalRef__p( env, &opaque->obj);
	YX_DeleteGlobalRef__p( env, &opaque->inputSurface);
	YX_DeleteGlobalRef__p( env, &opaque->inputBuffer);
	YX_DeleteGlobalRef__p( env, &opaque->inputBufferExt);
	YX_DeleteGlobalRef__p( env, &opaque->outputBuffer);
	free( opaque);
	opaque = NULL;

done:
	if( NULL != _ctx)
	{
		free(_ctx);
		_ctx = NULL;
	}
	if( status>0)
	{
		YX_JNI_DetachThreadEnv();
	}
}

int 	YX_AMediaCodec_Enc_initEncoder( YX_AMediaCodec_Enc * _ctx, int _width, int _height, int _frameRate, int _colorFormat, int _iFrameInterval, int _bitRate, int _profile, bool _useInputSurface, int _encType)
{
	YXLOGI("Into  YX_AMediaCodec_Enc_initEncoder");

	YX_AMediaCodec_Enc_Opaque * opaque = _ctx->opaque;
	JNIEnv * env = NULL;
	jobject  obj = opaque->obj;
	int iRet = -1;
	int status = YX_JNI_AttachThreadEnv(&env);
	if( status < 0)
	{
		YXLOGI( "YX_JNI_SetupThreadEnv failed!!!");
		goto fail;
	}
	opaque->enc_type = _encType;
	iRet = com_nxinc_VMediacodec_Enc__initEncoder( env, obj, _width, _height, _frameRate, _colorFormat, _iFrameInterval, _bitRate, _profile, _useInputSurface, _encType);
	YXLOGI("com_nxinc_VMediacodec_Enc__initEncoder w:[%d] h:[%d] frameRate:[%d] colorFormat:[%d] iFrameInterval:[%d] bitRate:[%d] return:[%d]",
			_width, _height, _frameRate, _colorFormat, _iFrameInterval, _bitRate, iRet);
	if( 0==iRet)
	{
		// 2 allocate memory
		jbyteArray tempInputBuffer 		= (*env)->NewByteArray( env, _width * _height * 3 / 2);
		jintArray  tempInputBufferExt 	= (*env)->NewIntArray( env, 10);
		jbyteArray tempOutputBuffer 	= (*env)->NewByteArray( env, _width * _height * 3 / 2);

		opaque->inputBuffer = (*env)->NewGlobalRef( env, tempInputBuffer);
		opaque->inputBufferExt = (*env)->NewGlobalRef( env, tempInputBufferExt);
		opaque->outputBuffer = (*env)->NewGlobalRef( env, tempOutputBuffer);

		(*env)->DeleteLocalRef( env, tempInputBuffer);
		(*env)->DeleteLocalRef( env, tempInputBufferExt);
		(*env)->DeleteLocalRef( env, tempOutputBuffer);

		opaque->colorFormat = _colorFormat;
		opaque->isNeedRefresh = true;
		// 设置是否开启异步编码;
		opaque->isUseAsyn = false;
		YX_AMediaCodec_Enc_refreshExtraData( _ctx);
	}
fail:
	if( status>0)
	{
		YX_JNI_DetachThreadEnv();
	}

	return iRet;
}

int 	YX_AMediaCodec_Enc_encodeVideoFromBuffer( YX_AMediaCodec_Enc * _ctx, uint8_t * _input, int _inputSize, uint8_t * _output, int _maxOutputSize, int * _pOutSize, int ouputFlag)
{
	YX_AMediaCodec_Enc_Opaque * opaque = _ctx->opaque;
	JNIEnv * env = NULL;
	jobject  obj = opaque->obj;
	int isKey = 0;
	long ret = 0;
	int status = YX_JNI_AttachThreadEnv(&env);
	if( status < 0)
	{
		YXLOGI( "YX_JNI_SetupThreadEnv failed!!!");
		return -1;
	}

	(*env)->SetByteArrayRegion( env, opaque->inputBuffer, 0, _inputSize, (const signed char*)_input);

	if(false==opaque->isUseAsyn)
	{
		ret = com_nxinc_VMediacodec_Enc__encodeVideoFromBuffer( env, opaque->obj, opaque->inputBuffer, opaque->outputBuffer);
	}
	else
	{
		ret = com_nxinc_VMediacodec_Enc__encodeVideoFromBufferAsyn( env, opaque->obj, opaque->inputBuffer, opaque->outputBuffer);
	}
	if( ret > 0 && ret < USR_DEF_MAX_PACKET_SIZE)
	{
		(*env)->GetByteArrayRegion( env, opaque->outputBuffer, 0, ret, (jbyte*)_output);
		if( (_output[4]&0x1f)==H264_NALU_TYPE_IDR_PICTURE || (_output[4]&0x1f)==H264_NALU_TYPE_SEI)
		{
			isKey |= 0x1;
		}
		*_pOutSize = ret;

		if ( ouputFlag)
		{
			convertH2645ExtraDataFlagToSize( _output, ret, opaque->enc_type);
		}

	}
	else
	{
		*_pOutSize = 0;
	}

	if( status>0)
	{
		YX_JNI_DetachThreadEnv();
	}

	return isKey;
}

int 	YX_AMediaCodec_Enc_encodeVideoFromTexture( YX_AMediaCodec_Enc * _ctx, int * _input, int _inputSize, uint8_t * _output, int _maxOutputSize, int * _pOutSize, int ouputFlag)
{
	YX_AMediaCodec_Enc_Opaque * opaque = _ctx->opaque;
	JNIEnv * env = NULL;
	jobject  obj = opaque->obj;
	int isKey = 0;
	long ret = 0;
	int status = YX_JNI_AttachThreadEnv(&env);
	if( status < 0)
	{
		YXLOGI( "YX_JNI_SetupThreadEnv failed!!!");
		return -1;
	}

	(*env)->SetIntArrayRegion( env, opaque->inputBufferExt, 0, _inputSize, _input);

	if(false==opaque->isUseAsyn)
	{
		ret = com_nxinc_VMediacodec_Enc__encodeVideoFromTexture( env, opaque->obj, opaque->inputBufferExt,  opaque->outputBuffer);
	}
	else
	{
		ret = com_nxinc_VMediacodec_Enc__encodeVideoFromTextureAsyn( env, opaque->obj, opaque->inputBufferExt,  opaque->outputBuffer);
	}

	if( ret > 0 && ret < USR_DEF_MAX_PACKET_SIZE)
	{
		(*env)->GetByteArrayRegion( env, opaque->outputBuffer, 0, ret, (jbyte*)_output);
		int val = _output[4]&0x1f;
		if( val==H264_NALU_TYPE_IDR_PICTURE || val==H264_NALU_TYPE_SEI)
		{
#ifdef USR_CODEC_DEBUG
//			YXLOGE( "is key frame!!!! size:[%l]", ret);
#endif
			isKey |= 0x1;
		}
		*_pOutSize = ret;

		if ( ouputFlag)
		{
			convertH2645ExtraDataFlagToSize( _output, ret, opaque->enc_type);
		}
	}
	else
	{
		*_pOutSize = 0;
	}

	if( status>0)
	{
		YX_JNI_DetachThreadEnv();
	}

	return isKey;
}

int 	YX_AMediaCodec_Enc_getLastFrameFlags(YX_AMediaCodec_Enc * _ctx)
{
    	YX_AMediaCodec_Enc_Opaque * opaque = _ctx->opaque;
    	JNIEnv * env = NULL;
    	jobject  obj = opaque->obj;
    	long ret = 0;
    	int status = YX_JNI_AttachThreadEnv(&env);
    	if( status < 0)
    	{
    		YXLOGI( "YX_JNI_SetupThreadEnv failed!!!");
    		return -1;
    	}

    	ret = com_nxinc_VMediacodec_Enc__getLastFramFlags( env, opaque->obj);

    	if( status>0)
    	{
    		YX_JNI_DetachThreadEnv();
    	}

    	return ret;
}



int 	YX_AMediaCodec_Enc_closeEncoder( YX_AMediaCodec_Enc * _ctx)
{

	YX_AMediaCodec_Enc_Opaque * opaque = _ctx->opaque;
	JNIEnv * env = NULL;
	YXLOGI( "Into YX_AMediaCodec_Enc_closeEncoder!!! opaque:[%p]", opaque);

	if( NULL == opaque)
	{
		YXLOGI( "_ctx->opaque == NULL !!!");
		return -1;
	}

	int status = YX_JNI_AttachThreadEnv(&env);
	if( status < 0)
	{
		YXLOGI( "YX_JNI_SetupThreadEnv failed!!!");
		return -1;
	}

	if( false==opaque->isUseAsyn)
	{
		com_nxinc_VMediacodec_Enc__closeEncoder( env, opaque->obj);
	}
	else
	{
		com_nxinc_VMediacodec_Enc__closeEncoderAsyn( env, opaque->obj);
	}

	YX_DeleteGlobalRef__p( env, &opaque->inputBuffer);
	YX_DeleteGlobalRef__p( env, &opaque->outputBuffer);

	YXLOGI( "Out YX_AMediaCodec_Enc_closeEncoder!!!");
	if( status>0)
	{
		YX_JNI_DetachThreadEnv();
	}

	return 0;
}

int 	YX_AMediaCodec_Enc_getSupportedColorFormat( YX_AMediaCodec_Enc * _ctx)
{
	YX_AMediaCodec_Enc_Opaque * opaque = _ctx->opaque;
	JNIEnv * env = NULL;
	jobject  obj = opaque->obj;

	int iRet = 0;
	int status = YX_JNI_AttachThreadEnv(&env);
	if( status < 0)
	{
		YXLOGI( "YX_JNI_SetupThreadEnv failed!!!");
		return -1;
	}

	iRet = com_nxinc_VMediacodec_Enc__getSupportedColorFormat( env, obj);

	if( status>0)
	{
		YX_JNI_DetachThreadEnv();
	}
	return iRet;
}

void 	YX_AMediaCodec_Enc_refreshExtraData( YX_AMediaCodec_Enc * _ctx)
{
	YX_AMediaCodec_Enc_Opaque * opaque = _ctx->opaque;
	JNIEnv * env = NULL;
	jobject  obj = opaque->obj;
	YXLOGI( "call YX_AMediaCodec_Enc_refreshExtraData [%p]!!!", obj);

//	if( false==opaque->isNeedRefresh)
//	{
//		return;
//	}

	int status = YX_JNI_AttachThreadEnv(&env);
	if( status < 0)
	{
		YXLOGI( "YX_JNI_SetupThreadEnv failed!!!");
		return;
	}

	int iSize = com_nxinc_VMediacodec_Enc__getExtraData( env, obj, opaque->outputBuffer);
	if( iSize>0)
	{
		opaque->headerSize = iSize;
		(*env)->GetByteArrayRegion( env, opaque->outputBuffer, 0, iSize, (jbyte*)opaque->headerData);
		opaque->isNeedRefresh = false;
		YXLOGI("YX_AMediaCodec_Enc_refreshExtraData!!!!");
	} else
	{
		YXLOGI("YX_AMediaCodec_Enc_refreshExtraData failed!!!");
	}
	if( status>0)
	{
		YX_JNI_DetachThreadEnv();
	}

}

int		YX_AMediaCodec_Enc_getExtraDataSize( YX_AMediaCodec_Enc * _ctx)
{
	YX_AMediaCodec_Enc_Opaque * opaque = _ctx->opaque;
	int iSize = opaque->headerSize;
	return iSize;
}

void 	YX_AMediaCodec_Enc_getExtraData( YX_AMediaCodec_Enc * _ctx, uint8_t * _extraData, int _maxSize)
{
	YX_AMediaCodec_Enc_Opaque * opaque = _ctx->opaque;
	if( NULL != opaque->headerData)
	{
		memcpy( _extraData, opaque->headerData, opaque->headerSize);
	}
	return;
}

void	YX_AMediaCodec_Enc_setEncoder( YX_AMediaCodec_Enc * _ctx, int _width, int _height, int _frameRate, int _bitRate, int _iFrameInterval, int _colorFormat, int _profile)
{
	YX_AMediaCodec_Enc_Opaque * opaque = _ctx->opaque;
	JNIEnv * env = NULL;
	jobject  obj = opaque->obj;
	int ret = 0;

//	YXLOGE( "call YX_AMediaCodec_Enc_setEncoder [%p] width:[%d] height:[%d] frameRate:[%d] bitRate:[%d] frameInternal:[%d] colorFormat:[%d]!!!",
//										   obj,     _width,    _height,    _frameRate,    _bitRate,   _iFrameInterval,   _colorFormat);

	int status = YX_JNI_AttachThreadEnv(&env);
	if( status < 0)
	{
		YXLOGI( "YX_JNI_SetupThreadEnv failed!!!");
		return;
	}

	com_nxinc_VMediacodec_Enc__setEncoder( env, obj, _width, _height, _frameRate, _bitRate, _iFrameInterval, _colorFormat, _profile);

	if( status>0)
	{
		YX_JNI_DetachThreadEnv();
	}
}

int		YX_AMediaCodec_Enc_getColorFormat( YX_AMediaCodec_Enc * _ctx)
{
	YX_AMediaCodec_Enc_Opaque * opaque = _ctx->opaque;
	if( NULL != opaque)
	{
		return opaque->colorFormat;
	}
	return 0;
}

int64_t	YX_AMediaCodec_Enc_getLastCodecPts( YX_AMediaCodec_Enc * _ctx)
{
	YX_AMediaCodec_Enc_Opaque * opaque = _ctx->opaque;
	JNIEnv * env = NULL;
	jobject  obj = opaque->obj;
	int nCount = 0;
	int64_t pts = -1;
//	YXLOGE( "call YX_AMediaCodec_Enc_getLastCodecPts [%p]!!!", obj);
	int status = YX_JNI_AttachThreadEnv(&env);
	if( status < 0)
	{
		YXLOGI( "YX_JNI_SetupThreadEnv failed!!!");
		return pts;
	}

	/*
	nCount = com_nxinc_VMediacodec_Enc__getInfoByFlag( env, obj, opaque->inputBufferExt, 1);
	if( nCount==2)
	{
		int tempBuf[2] = {0};
		(*env)->GetIntArrayRegion( env, opaque->inputBufferExt, 0, 2, &tempBuf[0]);
		pts = tempBuf[0];
	 	tempBuf[1]&0xFFFFFFFFL
		pts |= ((int64_t)(tempBuf[1]&0xFFFFFFFF)<<32);
	}
	 */
	pts = com_nxinc_VMediacodec_Enc__getLastPts(env, obj);


	if( status>0)
	{
		YX_JNI_DetachThreadEnv();
	}

	return pts;
}
