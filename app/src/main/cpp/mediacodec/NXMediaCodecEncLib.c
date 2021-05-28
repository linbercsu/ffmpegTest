/*
 * com_nxinc_mediacodec_lib.c
 *
 *  Created on: 2016-7-27
 *      Author:
 */


#include "libavutil/attributes.h"
#include "libavutil/common.h"
#include "libavutil/opt.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/mathematics.h"
#include "libavcodec/internal.h"

#include "libavcodec/avcodec.h"
//#include "libavcodec/codec.h"
#include "android/JniUtils/NXAndroidJni.h"
#include "NXMediaCodecEncInterface.h"
#include "NXUtilCodecInfoProcess.h"

#include <unistd.h>

#define THIS_FILE "YXMediaCodecLib"

#undef YX_LOG_TAG
#define YX_LOG_TAG THIS_FILE

#define COLOR_FormatSurface 			0x7F000789
#define COLOR_FormatYUV420Planar		19
#define COLOR_FormatYUV420SemiPlanar	21

#define TEST_DYNAMIC_PARAM_FLAG 0
#if TEST_DYNAMIC_PARAM_FLAG
#define TEST_DEF_CHANGE_STEP_FRAMES 200
#define TEST_DEF_MAX_BITRATE 7500000
#define TEST_DEF_MIN_BITRATE 300000
#define TEST_DEF_STEP_BITRATE 500000
static int 	sv_iFrameCount = 0;
static bool sv_bIncrease = true;
static int 	sv_iCurrentBitRate = 0;
#endif

#define TEST_ENABLE_MEM_ENC_TEST 0

#define LABEL_FAIL fail


// 图像缓存用;
#define USR_DEF_MAX_CACHE_BUF_SIZE (1920*1080*2)
#define USR_DEF_MAX_HEADER_BUF_SIZE 1024

#define MIME_VIDEO_ENC_TYPE_H264 0
#define MIME_VIDEO_ENC_TYPE_HEVC 1

const static AVRational ANDROID_TIMEBASE = (AVRational){1, 1000000};

typedef struct _sAndroidHwContext {
    const AVClass *av_class;
    void *encoder;
    char *profile;
	char *useavcformat;
	char *bitRate;
	char *frameRate;
	char *useinputsurface;
	char * mp4_use;
	AVFrame * lastFrame;
    int   flag;
	unsigned char slv_pbtCacheImgBuf[USR_DEF_MAX_CACHE_BUF_SIZE];
	int slv_iLogFlag;

	unsigned char slv_pbtSwapImgBuf[USR_DEF_MAX_CACHE_BUF_SIZE];

	unsigned char slv_pbtIDRHeaderData[USR_DEF_MAX_HEADER_BUF_SIZE];
	int slv_iIDRHeaderSize;
	bool sv_bRefreshedIDRHeader;
	bool useForMp4;
    bool globalHeader;
    int64_t lastPts;
}SAndroidHwContext;

typedef struct _SH264HWParam
 {
     int iWidth;
     int iHeight;
     int iColor;
     int iFrameRate;
     int iProfile;
     int iBitrate;  //bps
     int iFramenternal;
 }SH264HWParam;

 typedef struct YXTextureFrameST
 {
	 int i64TextureID;
	 int i64SharedEGLContext;

 }YXTextureFrameST;

 typedef struct _SHWImageFrame
 {
 	int iWidth;
 	int iHeight;
 	int	iDataSize;
 	int iImgFormat;

     void* pYBuf;
     int iYStride;

     void* pUBuf;
     int iUStride;

     void* pVBuf;
     int iVStride;

     int64_t qPTS;
 }SHWImageFrame;

#define OFFSET(x) offsetof(SAndroidHwContext, x)
#define VE AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM
static const AVOption options[] = {
	    { "profile", 	"Set profile restrictions", 		OFFSET(profile), 		AV_OPT_TYPE_STRING, 	{ 0 }, 0, 0, VE },
		{ "useavcformat", "Set if use avcc format extradata", OFFSET(useavcformat), AV_OPT_TYPE_STRING, 	{ 0 }, 0, 0, VE },
		{ "bitRate", 	"Set bitRate restrictions", 		OFFSET(bitRate), 		AV_OPT_TYPE_STRING, 	{ 0 }, 0, 0, VE },
	    { "frameRate", 	"Set frameRate restrictions", 		OFFSET(frameRate), 		AV_OPT_TYPE_STRING, 	{ 0 }, 0, 0, VE },
		{ "useinputsurface", "Set if use a surface for input", OFFSET(useinputsurface), AV_OPT_TYPE_STRING, 	{ 0 }, 0, 0, VE },
		{ "mp4_use", "Set if use data for mp4 muxer", OFFSET(mp4_use), AV_OPT_TYPE_STRING, 	{ 0 }, 0, 0, VE },
	    { NULL }
};

static const AVClass class = {
    "libandroidhwh264enc", av_default_item_name, options, LIBAVUTIL_VERSION_INT
};

// 函数声明;
static av_cold int android_hw_encode_init(AVCodecContext *avctx);
static av_cold int android_hw_encode_close(AVCodecContext *avctx);
static int android_hw_encode_frame(AVCodecContext *avctx, AVPacket *avpkt, const AVFrame *frame, int *got_packet);
static void CopyAVFrameToMyFrame(  SAndroidHwContext *s, const AVFrame * _pFrame, int _srcFormat, int _dstFormat, SHWImageFrame * _pImgFrame);


static int refreshExtraData(SAndroidHwContext * s, AVCodecContext * avctx)
{

	int iExtraDataSize;
	YX_AMediaCodec_Enc_refreshExtraData( s->encoder);
	iExtraDataSize = YX_AMediaCodec_Enc_getExtraDataSize( s->encoder);
	if (iExtraDataSize > 0)
	{
		if ( s->globalHeader)
		{
			if( avctx->extradata_size < iExtraDataSize)
			{
				if ( avctx->extradata != NULL)
				{
					av_free( avctx->extradata);
					avctx->extradata = NULL;
				}
				YXLOGE(  "avctx->extradata = av_mallocz %d + FF_INPUT_BUFFER_PADDING_SIZE", iExtraDataSize);

//				avctx->extradata = av_mallocz(iExtraDataSize + FF_INPUT_BUFFER_PADDING_SIZE);
				avctx->extradata = av_mallocz(iExtraDataSize + AV_INPUT_BUFFER_PADDING_SIZE);
			}

			if (!avctx->extradata)
			{
				goto LABEL_FAIL;
			}

			avctx->extradata_size = iExtraDataSize;
			YX_AMediaCodec_Enc_getExtraData(s->encoder, avctx->extradata, avctx->extradata_size);

			if(s->useavcformat && !strcmp(s->useavcformat, "true"))
			{
				if ( avctx->codec_id == AV_CODEC_ID_H264)
				{
                    processExtraData2AVCC(avctx->extradata, &avctx->extradata_size);
				}
				else if ( avctx->codec_id == AV_CODEC_ID_HEVC)
				{
                    processExtraData2HVCC(avctx->extradata, &avctx->extradata_size, avctx);
				}

			}
		}

		s->slv_iIDRHeaderSize = iExtraDataSize;
		YX_AMediaCodec_Enc_getExtraData(s->encoder, s->slv_pbtIDRHeaderData, s->slv_iIDRHeaderSize);
		if ( s->useForMp4)
		{
			if ( avctx->codec_id == AV_CODEC_ID_HEVC)
			{
				convertH2645ExtraDataFlagToSize( s->slv_pbtIDRHeaderData, s->slv_iIDRHeaderSize, MIME_VIDEO_ENC_TYPE_HEVC);
			} else
			{
				convertH2645ExtraDataFlagToSize( s->slv_pbtIDRHeaderData, s->slv_iIDRHeaderSize, MIME_VIDEO_ENC_TYPE_H264);
			}
		}

//		YX_Dump_Hex( avctx->extradata, avctx->extradata_size, 10);
	}
    else
    {
        s->slv_iIDRHeaderSize = 0;
        goto LABEL_FAIL;
    }
    return 0;

LABEL_FAIL:
	return -1;
}

#if TEST_ENABLE_MEM_ENC_TEST
static av_cold int android_hw_encode_init(AVCodecContext *avctx)
{
	YXLOGI("Into TEST_ENABLE_MEM_ENC_TEST android_hw_encode_init!!!");

	SAndroidHwContext *s = (SAndroidHwContext *)avctx->priv_data;
	SH264HWParam param = { 0 };
	enum AVPixelFormat srcColor = AV_PIX_FMT_YUV420P;

#else
static av_cold int android_hw_encode_init(AVCodecContext *avctx)
{
	YXLOGI("Into android_hw_encode_init!!!");

	SAndroidHwContext *s = (SAndroidHwContext *)avctx->priv_data;
    SH264HWParam param = { 0 };
	enum AVPixelFormat srcColor = avctx->pix_fmt;
#endif
   int err = AVERROR_UNKNOWN;
	JNIEnv * env = NULL;
	int status = YX_JNI_AttachThreadEnv( &env);
	s->sv_bRefreshedIDRHeader = false;
	s->lastPts = 0;
	if( status<0)
	{
		return -1;
	}

	s->useForMp4 = false;
    s->globalHeader = (avctx->flags & AV_CODEC_FLAG_GLOBAL_HEADER) == 0 ? false : true;
	if ( s->mp4_use != NULL && !strcmp( s->mp4_use, "true"))
	{
		s->useForMp4 = true;
	}

	if( srcColor==AV_PIX_FMT_VIDEOTOOLBOX && YX_Android_GetApiLevel()<YX_API_18_JELLY_BEAN_MR2)
	{
		YXLOGE( "Surface encoder need SDK_INT >= 18 \n");
		goto fail;
	}

//	if( YX_AMediaCodec_Enc_isInNotSupportedList())
//	{
//		YXLOGI( "The device is in NOT SUPPORTED list, please check list for detail!!!");
//		goto fail;
//	}
	s->lastFrame = av_frame_alloc();
	s->lastFrame->opaque = NULL;
    s->flag = 0;
    s->encoder = YX_AMediaCodec_Enc_createEncoderObject();
    if( NULL==s->encoder)
    {
    	YXLOGI( "YX_AMediaCodec_Enc_createEncoderObject failed  !!! \n");
    	goto fail;
    }

    param.iFrameRate   = avctx->framerate.num / avctx->framerate.den;
    param.iWidth       = avctx->width;
    param.iHeight      = avctx->height;
    param.iBitrate     = (int)avctx->bit_rate;
    if (param.iBitrate == 0) {
        param.iBitrate = 700000;
    }
    param.iColor= YX_AMediaCodec_Enc_getSupportedColorFormat(s->encoder);
    YXLOGI( "YX_AMediaCodec_Enc_getSupportedColorFormat:[%d] !!! \n", param.iColor);

    param.iProfile = 0;
    if (s->profile && !strcmp(s->profile, "main"))
    {
        param.iProfile = 1;
    }

    // 这个值乱设会挂的;
    param.iFramenternal  = 1;
	if ( avctx->gop_size > 0)
	{
		param.iFramenternal = (1+avctx->gop_size)/param.iFrameRate;
        if ( param.iFramenternal < 1)
        {
            param.iFramenternal  = 1;
        }

    } else
    {
        param.iFramenternal = 0;
    }


    if( param.iColor <0)
    {
    	err = param.iColor;
    	goto fail;
    }

    if( srcColor==AV_PIX_FMT_VIDEOTOOLBOX)
    {
    	param.iColor = COLOR_FormatSurface;
        err = YX_AMediaCodec_Enc_initEncoder(s->encoder, param.iWidth, param.iHeight,
											 param.iFrameRate, param.iColor,
											 param.iFramenternal, param.iBitrate,
											 param.iProfile,
											 true,
											 MIME_VIDEO_ENC_TYPE_H264);

    }
    else
    {
        err = YX_AMediaCodec_Enc_initEncoder(s->encoder, param.iWidth, param.iHeight,
											 param.iFrameRate, param.iColor,
											 param.iFramenternal, param.iBitrate,
											 param.iProfile,
											 false,
											 MIME_VIDEO_ENC_TYPE_H264);

    }

	if( err <0)
	{
		goto fail;
	}

//	YXLOGE( "option bitrate:[%s] frameRate:[%s] useInputsurface:[%s]", s->bitRate, s->frameRate, s->useinputsurface);

    if (1)//avctx->flags & CODEC_FLAG_GLOBAL_HEADER)
    {
		avctx->extradata = NULL;
		avctx->extradata_size = 0;
//		refreshExtraData( s, avctx);
    }


#if TEST_DYNAMIC_PARAM_FLAG
    sv_iFrameCount = 0;
    sv_iCurrentBitRate = param.iBitrate;
#endif

    if( status>0)
    {
    	YX_JNI_DetachThreadEnv();
    }

    YXLOGI(  "android_hw_encode_init exit success !!! \n");

    return 0;

fail:
YXLOGI(  "android_hw_encode_init exit failed !!! \n");
   if( status>0)
   {
	   YX_JNI_DetachThreadEnv();
   }
   android_hw_encode_close(avctx);
   return err;
}

#if 0
#if TEST_ENABLE_MEM_ENC_TEST
static av_cold int android_hw_encode_init(AVCodecContext *avctx)
{
	YXLOGI("Into TEST_ENABLE_MEM_ENC_TEST android_hw_encode_init!!!");

	SAndroidHwContext *s = (SAndroidHwContext *)avctx->priv_data;
	SH264HWParam param = { 0 };
	enum AVPixelFormat srcColor = AV_PIX_FMT_YUV420P;

#else
static av_cold int android_hw_encode_init_hevc(AVCodecContext *avctx)
{
	YXLOGI("Into android_hw_encode_init_hevc!!!");

	SAndroidHwContext *s = (SAndroidHwContext *)avctx->priv_data;
	SH264HWParam param = { 0 };
	enum AVPixelFormat srcColor = avctx->pix_fmt;
#endif
	int err = AVERROR_UNKNOWN;
	JNIEnv * env = NULL;
	int status = YX_JNI_AttachThreadEnv( &env);
	s->sv_bRefreshedIDRHeader = false;
	if( status<0)
	{
		return -1;
	}

	s->useForMp4 = false;
	s->globalHeader = (avctx->flags & AV_CODEC_FLAG_GLOBAL_HEADER) == 0 ? false : true;
	if ( s->mp4_use != NULL && !strcmp( s->mp4_use, "true"))
	{
		s->useForMp4 = true;
	}

	if( srcColor==AV_PIX_FMT_VIDEOTOOLBOX && YX_Android_GetApiLevel()<YX_API_18_JELLY_BEAN_MR2)
	{
		YXLOGE( "Surface encoder need SDK_INT >= 18 \n");
		goto fail;
	}

//	if( YX_AMediaCodec_Enc_isInNotSupportedList())
//	{
//		YXLOGI( "The device is in NOT SUPPORTED list, please check list for detail!!!");
//		goto fail;
//	}
	s->lastFrame = av_frame_alloc();
	s->lastFrame->opaque = NULL;
	s->flag = 0;
	s->encoder = YX_AMediaCodec_Enc_createEncoderObject();
	if( NULL==s->encoder)
	{
		YXLOGI( "YX_AMediaCodec_Enc_createEncoderObject failed  !!! \n");
		goto fail;
	}

	param.iFrameRate   = avctx->framerate.num / avctx->framerate.den;
	param.iWidth       = avctx->width;
	param.iHeight      = avctx->height;
	param.iBitrate     = (int)avctx->bit_rate;
	if (param.iBitrate == 0) {
		param.iBitrate = 700000;
	}
	param.iColor= YX_AMediaCodec_Enc_getSupportedColorFormat(s->encoder);
	YXLOGI( "YX_AMediaCodec_Enc_getSupportedColorFormat:[%d] !!! \n", param.iColor);

	param.iProfile = 0;
	if (s->profile && !strcmp(s->profile, "main"))
	{
		param.iProfile = 1;
	}

	// 这个值乱设会挂的;
	param.iFramenternal  = 1;
	if ( param.iFrameRate < 1)
	{
		param.iFrameRate = 25;
	}
	if ( avctx->gop_size > 0)
	{
		param.iFramenternal = (1+avctx->gop_size)/param.iFrameRate;
		if ( param.iFramenternal < 1)
		{
			param.iFramenternal  = 1;
		}

	} else
	{
		param.iFramenternal = 0;
	}


	if( param.iColor <0)
	{
		err = param.iColor;
		goto fail;
	}

	if( srcColor==AV_PIX_FMT_VIDEOTOOLBOX)
	{
		param.iColor = COLOR_FormatSurface;
		err = YX_AMediaCodec_Enc_initEncoder(s->encoder, param.iWidth, param.iHeight,
											 param.iFrameRate, param.iColor,
											 param.iFramenternal,
											 param.iBitrate, param.iProfile,
											 true,
											 MIME_VIDEO_ENC_TYPE_HEVC);

	}
	else
	{
		err = YX_AMediaCodec_Enc_initEncoder(s->encoder, param.iWidth, param.iHeight,
											 param.iFrameRate, param.iColor,
											 param.iFramenternal, param.iBitrate,
											 param.iProfile,
											 false,
											 MIME_VIDEO_ENC_TYPE_HEVC);

	}

	if( err <0)
	{
		goto fail;
	}

//	YXLOGE( "option bitrate:[%s] frameRate:[%s] useInputsurface:[%s]", s->bitRate, s->frameRate, s->useinputsurface);

	if (1)//avctx->flags & CODEC_FLAG_GLOBAL_HEADER)
	{
		avctx->extradata = NULL;
		avctx->extradata_size = 0;
//		refreshExtraData( s, avctx);
	}


#if TEST_DYNAMIC_PARAM_FLAG
	sv_iFrameCount = 0;
    sv_iCurrentBitRate = param.iBitrate;
#endif

	if( status>0)
	{
		YX_JNI_DetachThreadEnv();
	}

	YXLOGI(  "android_hw_encode_init exit success !!! \n");

	return 0;

	fail:
	YXLOGI(  "android_hw_encode_init exit failed !!! \n");
	if( status>0)
	{
		YX_JNI_DetachThreadEnv();
	}
	android_hw_encode_close(avctx);
	return err;
}
#endif
static int checkFrameParam( int fmt)
{
	int nRet = -1;
	switch (fmt)
	{
		case AV_PIX_FMT_VIDEOTOOLBOX:
		case AV_PIX_FMT_YUV420P:
		case AV_PIX_FMT_NV12:
		case AV_PIX_FMT_NV21:
			nRet = 0;
			break;
		default:
			break;
	}

	return nRet;
}

static av_cold int android_hw_encode_close(AVCodecContext *avctx)
{
	YXLOGI( "GETID %s thread tid:[%d] gid:[%d] uid:[%d]!!!\n", __func__ ,gettid(),getegid(), getuid());

	SAndroidHwContext *s = avctx->priv_data;
	JNIEnv * env = NULL;
	int status = YX_JNI_AttachThreadEnv( &env);
	if( status<0)
	{
		return -1;
	}

	if( NULL != s && NULL != s->encoder)
	{
		YX_AMediaCodec_Enc_closeEncoder( s->encoder);
		YX_AMediaCodec_Enc_destoryEncoderObject( s->encoder);
		s->encoder = NULL;
	}

	if ( s->lastFrame != NULL)
	{
		av_frame_free( &s->lastFrame);
		s->lastFrame = NULL;
	}



    if( status>0)
    {
    	YX_JNI_DetachThreadEnv();
    }

    return 0;
}


#if TEST_ENABLE_MEM_ENC_TEST
static int android_hw_encode_frame(AVCodecContext *avctx, AVPacket *avpkt,
								   const AVFrame *frame, int *got_packet)
{
	enum AVPixelFormat srcColor = AV_PIX_FMT_YUV420P;

#else
	static int android_hw_encode_frame(AVCodecContext *avctx, AVPacket *avpkt,
                            const AVFrame *frame, int *got_packet)
{
	enum AVPixelFormat srcColor = avctx->pix_fmt;
#endif
    SAndroidHwContext *s = avctx->priv_data;
    int ret;
    int nFlags = 0;
    SHWImageFrame sFrame;
	JNIEnv * pEnv = NULL;
	const uint8_t * pData = NULL;
	int dstFormat = YX_AMediaCodec_Enc_getColorFormat(s->encoder);
	int srcFormat = 0;
	int64_t currentPts = s->lastPts + 1;
	if (frame != NULL)
		currentPts = av_rescale_q(frame->pts, avctx->time_base, ANDROID_TIMEBASE);

	int status = YX_JNI_AttachThreadEnv( &pEnv);
	if( status<0)
	{
		return -1;
	}

//	YXLOGE("android_hw_encode_frame in fmt:[%d] srcColor:[%d] linesize0:[%d] data[0]:[%p]!!!", frame->format, srcColor, frame->linesize[0], frame->data[0]);

#if USR_CODEC_DEBUG
//	YXLOGE("android_hw_encode_frame in fmt:[%d] srcColor:[%d] linesize0:[%d] data[0]:[%p]!!!", frame->format, srcColor, frame->linesize[0], frame->data[0]);
#endif
	if( NULL != frame && 0==checkFrameParam(frame->format))
	{
#if TEST_ENABLE_MEM_ENC_TEST
		srcFormat = AV_PIX_FMT_YUV420P;
		CopyAVFrameToMyFrame( frame, srcFormat, COLOR_FormatYUV420Planar, &sFrame);
		sFrame.iImgFormat = dstFormat;
#else
		srcFormat = frame->format;
		CopyAVFrameToMyFrame( s, frame, srcFormat, dstFormat, &sFrame);
#endif
	}
	else
	{
		srcFormat = srcColor;
		memset(&sFrame, 0, sizeof(sFrame));
        sFrame.pYBuf = &s->slv_pbtCacheImgBuf[0];
        sFrame.iYStride = sizeof(YXTextureFrameST)/sizeof(int);
        memset( s->slv_pbtCacheImgBuf, 0, sizeof(YXTextureFrameST));
	}

	if( 1)
	{
		int frameRate = -1;
		int bitRate = -1;
		if( NULL!= s->frameRate)
		{
			frameRate = atoi(s->frameRate);
		}
		if( NULL != s->bitRate)
		{
			bitRate = atoi(s->bitRate);
		}
		YX_AMediaCodec_Enc_setEncoder( s->encoder, -1, -1, frameRate, bitRate, -1, -1, -1);
//		YXLOGE( "option bitrate:[%d] frameRate:[%d] ", bitRate, frameRate);
	}

	if(1)
	{
	    int iPacketSize = 0;
	    int lv_iMaxSwapSize = USR_DEF_MAX_CACHE_BUF_SIZE;
	    void * pSwapBuf = &s->slv_pbtSwapImgBuf[0];
		int skipSize = 0;
	    if( NULL==pSwapBuf)
	    {
	    	ret = AVERROR(ENOMEM);
	    	goto done;
	    }

	    if( AV_PIX_FMT_VIDEOTOOLBOX==srcFormat)
	    {
			int *  pData = (int * )sFrame.pYBuf;

#if TESR_RUNTIME_COST==1
			rc_gloable_time_start();
#endif

		    nFlags = YX_AMediaCodec_Enc_encodeVideoFromTexture( s->encoder, pData, sFrame.iYStride, pSwapBuf, lv_iMaxSwapSize, &iPacketSize, s->useForMp4);

#if TESR_RUNTIME_COST==1
		    rc_gloable_time_stop();
		    rc_gloable_statistic_add();
		    YXLOGE("%s", rc_gloable_time_log_str());
#endif
	    }
	    else
	    {
			uint8_t *  pData = (uint8_t * )sFrame.pYBuf;
			nFlags = YX_AMediaCodec_Enc_encodeVideoFromBuffer( s->encoder, pData, sFrame.iDataSize, pSwapBuf, lv_iMaxSwapSize, &iPacketSize, s->useForMp4, currentPts);
	    }
	    *got_packet = 0;
	    if (iPacketSize <= 0) {
	        ret = 0;
	        goto done;
	    }
		skipSize = 0;
		if ( nFlags&1)
		{
			if ( false == s->sv_bRefreshedIDRHeader)
			{
				refreshExtraData( s, avctx);
				if( s->slv_iIDRHeaderSize > 0)
				{
					s->sv_bRefreshedIDRHeader = true;
				}
			}
			skipSize = s->slv_iIDRHeaderSize;
		}

		ret = av_new_packet( avpkt, iPacketSize + skipSize);
//	    ret = ff_alloc_packet2( avctx, avpkt, iPacketSize + skipSize, iPacketSize + s->slv_iIDRHeaderSize + AV_INPUT_BUFFER_PADDING_SIZE);
	    if (ret<0) {
	    	YXLOGI( "ff_alloc_packet:[%d] ret:[%d] failed\n", iPacketSize, ret);
	        goto done;
	    }

	    avpkt->pts = YX_AMediaCodec_Enc_getLastCodecPts( s->encoder);
//        __android_log_print(6, "enc", "video pts: %ld, %d, %d", avpkt->pts, num, den);
		avpkt->pts = av_rescale_q(avpkt->pts, ANDROID_TIMEBASE, avctx->time_base);
//        avpkt->pts = avpkt->pts*den/num/1000000;
	    avpkt->dts = avpkt->pts;
		nFlags = YX_AMediaCodec_Enc_getLastFrameFlags( s->encoder);
	    if (nFlags&1)
	    {
	        avpkt->flags = AV_PKT_FLAG_KEY;
			if ( skipSize > 0)
			{
				memcpy( avpkt->data, s->slv_pbtIDRHeaderData, skipSize);
			}
			memcpy( avpkt->data+skipSize, pSwapBuf, iPacketSize);

#if USR_CODEC_DEBUG
			YXLOGE(  "android_hw_encode_frame succeed !!! size:[%d] key:[%d] pts:[%lld] iPacketSize:[%d]", avpkt->size, nFlags, avpkt->dts, iPacketSize);
#endif
	    }
	    else
	    {
			avpkt->flags = 0;
#if USR_CODEC_DEBUG
			YXLOGI(  "android_hw_encode_frame succeed !!! size:[%d] key:[%d] pts:[%lld] iPacketSize:[%d]", avpkt->size, nFlags, avpkt->dts, iPacketSize);
#endif
			memcpy( avpkt->data, pSwapBuf, iPacketSize);
		}

	    *got_packet = 1;


	}

done:
	if( status>0)
	{
		YX_JNI_DetachThreadEnv();
	}

    return ret;
}

static int android_hw_send_frame(AVCodecContext *avctx, const AVFrame *frame)
{
	SAndroidHwContext *s = avctx->priv_data;
    int ret = -1;
	if ( NULL != s)
	{
        YXLOGE("call :  %s, line: %d, flags: %d, frame: %p",__FUNCTION__, __LINE__, s->flag, frame);
        if ( s->flag == 0 )
        {
			if ( NULL != frame)
			{
				av_frame_ref( s->lastFrame, frame);
				s->flag = 1;
			} else
			{
				s->flag = 2;
			}

			ret = 0;
		}
        else
        {
			if ( s->flag == 1)
			{
				ret = AVERROR(EAGAIN);
			}
			else
			{
				ret = AVERROR_EOF;
			}
        }
	}

	return ret;
}

static int android_hw_recive_packet(AVCodecContext *avctx, AVPacket *avpkt)
{
	SAndroidHwContext *s = avctx->priv_data;
	int got_packet;
	int ret = -1;
	if ( NULL != s)
	{
		if ( s->flag == 1)
		{
			ret = android_hw_encode_frame( avctx, avpkt, s->lastFrame, &got_packet);
			av_frame_unref(s->lastFrame);
			s->lastFrame->opaque = NULL;
			s->flag = 0;

			if ( ret >= 0)
			{
				if ( !got_packet)
				{
					ret = AVERROR(EAGAIN);
				}
			}
		}
		else if( s->flag == 2)
		{
			ret = android_hw_encode_frame( avctx, avpkt, NULL, &got_packet);
            if ( !got_packet)
            {
                ret = AVERROR_EOF;
            }
		}
		else
		{
			ret = AVERROR(EAGAIN);
		}

	}

	return ret;
}




static void CopyI420FrameToImageFrame( SAndroidHwContext *s, SHWImageFrame * pm_pImgFrame, const AVFrame * pm_pFrame)
{
	unsigned char * lv_pbtDst = &s->slv_pbtCacheImgBuf[0];
	unsigned char * lv_pbtSrc = pm_pFrame->data[0];
	int lv_iDstWidth 	= pm_pFrame->width;
	int lv_iDstHeight 	= pm_pFrame->height;
	int lv_iDstHalfWidth = (lv_iDstWidth+1)/2;
	int lv_iDstHalfHeight= lv_iDstHeight/2;

#if TEST_ENABLE_MEM_ENC_TEST==0
	int i, j;

	for( i=0; i<lv_iDstHeight; ++i)
	{
		memcpy( lv_pbtDst, lv_pbtSrc, lv_iDstWidth);
		lv_pbtDst += lv_iDstWidth;
		lv_pbtSrc += pm_pFrame->linesize[0];
	}

	unsigned char * lv_pbtSrcU = pm_pFrame->data[1];
	unsigned char * lv_pbtSrcV = pm_pFrame->data[2];
	int lv_iUSkip = pm_pFrame->linesize[1] - lv_iDstHalfWidth;
	int lv_iVSkip = pm_pFrame->linesize[2] - lv_iDstHalfWidth;
	for( i=0; i<lv_iDstHalfHeight; ++i)
	{
		for( j=0; j<lv_iDstHalfWidth; ++j)
		{
			*lv_pbtDst++ = *lv_pbtSrcU++;
			*lv_pbtDst++ = *lv_pbtSrcV++;
		}
		lv_pbtSrcU += lv_iUSkip;
		lv_pbtSrcV += lv_iVSkip;
	}
#endif

    SHWImageFrame sFrame;
    sFrame.pYBuf = &s->slv_pbtCacheImgBuf[0];
    sFrame.pUBuf = sFrame.pYBuf + lv_iDstWidth*lv_iDstHeight;
    sFrame.pVBuf = sFrame.pUBuf + lv_iDstHalfWidth*lv_iDstHalfHeight;

    sFrame.iYStride = lv_iDstWidth;
    sFrame.iUStride = lv_iDstHalfWidth;
    sFrame.iVStride = lv_iDstHalfWidth;

    sFrame.qPTS = pm_pFrame->pts;

    sFrame.iWidth	= lv_iDstWidth;
    sFrame.iHeight	= lv_iDstHeight;
    sFrame.iDataSize= sFrame.iYStride*sFrame.iHeight + sFrame.iUStride*sFrame.iHeight;

    *pm_pImgFrame = sFrame;
//	YXLOGE("CopyI420FrameToImageFrame frame->format = [YUV420P]");

}

static void CopyNV21FrameToImageFrame( SAndroidHwContext *s, SHWImageFrame * pm_pImgFrame, const AVFrame * pm_pFrame)
{
	unsigned char * lv_pbtDst = &s->slv_pbtCacheImgBuf[0];
	unsigned char * lv_pbtSrc = pm_pFrame->data[0];
	int lv_iDstWidth 	= pm_pFrame->width;
	int lv_iDstHeight 	= pm_pFrame->height;
	int lv_iDstHalfWidth = (lv_iDstWidth+1)/2;
	int lv_iDstHalfHeight= lv_iDstHeight/2;
	int i, j;

	for( i=0; i<lv_iDstHeight; ++i)
	{
		memcpy( lv_pbtDst, lv_pbtSrc, lv_iDstWidth);
		lv_pbtDst += lv_iDstWidth;
		lv_pbtSrc += pm_pFrame->linesize[0];
	}

	unsigned char * lv_pbtSrcUV = pm_pFrame->data[1];
	int lv_iDstUVWidth = lv_iDstHalfWidth*2;
	int lv_iUVSkip = pm_pFrame->linesize[1] - lv_iDstUVWidth;
	for( i=0; i<lv_iDstHalfHeight; ++i)
	{
		for( j=0; j<lv_iDstUVWidth; j+=2)
		{
			lv_pbtDst[0] = lv_pbtSrcUV[1];
			lv_pbtDst[1] = lv_pbtSrcUV[0];
			lv_pbtDst 	+= 2;
			lv_pbtSrcUV += 2;
		}
		lv_pbtSrcUV += lv_iUVSkip;
	}


    SHWImageFrame sFrame;
    sFrame.pYBuf = &s->slv_pbtCacheImgBuf[0];
    sFrame.pUBuf = sFrame.pYBuf + lv_iDstWidth*lv_iDstHeight;
    sFrame.pVBuf = NULL;//sFrame.pUBuf + lv_iDstHalfWidth*lv_iDstHalfHeight;

    sFrame.iYStride = lv_iDstWidth;
    sFrame.iUStride = lv_iDstUVWidth;
    sFrame.iVStride = 0;

    sFrame.qPTS = pm_pFrame->pts;

    sFrame.iWidth	= lv_iDstWidth;
    sFrame.iHeight	= lv_iDstHeight;
    sFrame.iDataSize= sFrame.iYStride*sFrame.iHeight + sFrame.iUStride*lv_iDstHalfHeight;

    *pm_pImgFrame = sFrame;

}

static void CopyNV12FrameToImageFrame( SAndroidHwContext *s, SHWImageFrame * pm_pImgFrame, const AVFrame * pm_pFrame)
{
	unsigned char * lv_pbtDst = &s->slv_pbtCacheImgBuf[0];
	unsigned char * lv_pbtSrc = pm_pFrame->data[0];
	int lv_iDstWidth 	= pm_pFrame->width;
	int lv_iDstHeight 	= pm_pFrame->height;
	int lv_iDstHalfWidth = (lv_iDstWidth+1)/2;
	int lv_iDstHalfHeight= lv_iDstHeight/2;
	int i,j;
	for( i=0; i<lv_iDstHeight; ++i)
	{
		memcpy( lv_pbtDst, lv_pbtSrc, lv_iDstWidth);
		lv_pbtDst += lv_iDstWidth;
		lv_pbtSrc += pm_pFrame->linesize[0];
	}

	unsigned char * lv_pbtSrcUV = pm_pFrame->data[1];
	int lv_iDstUVWidth = lv_iDstHalfWidth*2;
	int lv_iUVSkip = pm_pFrame->linesize[1] - lv_iDstUVWidth;
	for( i=0; i<lv_iDstHalfHeight; ++i)
	{
		memcpy( lv_pbtDst, lv_pbtSrcUV, lv_iDstUVWidth);
		lv_pbtDst += lv_iDstUVWidth;
		lv_pbtSrcUV += pm_pFrame->linesize[1];
	}


    SHWImageFrame sFrame;
    sFrame.pYBuf = &s->slv_pbtCacheImgBuf[0];
    sFrame.pUBuf = sFrame.pYBuf + lv_iDstWidth*lv_iDstHeight;
    sFrame.pVBuf = NULL;//sFrame.pUBuf + lv_iDstHalfWidth*lv_iDstHalfHeight;

    sFrame.iYStride = lv_iDstWidth;
    sFrame.iUStride = lv_iDstUVWidth;
    sFrame.iVStride = 0;

    sFrame.qPTS = pm_pFrame->pts;

    sFrame.iWidth	= lv_iDstWidth;
    sFrame.iHeight	= lv_iDstHeight;
    sFrame.iDataSize= sFrame.iYStride*sFrame.iHeight + sFrame.iUStride*lv_iDstHalfHeight;

    *pm_pImgFrame = sFrame;

}


static void CopyI420ToI420ImageFrame(SAndroidHwContext *s, SHWImageFrame * pm_pImgFrame, const AVFrame * pm_pFrame)
{
	unsigned char * lv_pbtDst = &s->slv_pbtCacheImgBuf[0];
	unsigned char * lv_pbtSrc = pm_pFrame->data[0];
	int lv_iDstWidth 	= pm_pFrame->width;
	int lv_iDstHeight 	= pm_pFrame->height;
	int lv_iDstHalfWidth = (lv_iDstWidth+1)/2;
	int lv_iDstHalfHeight= lv_iDstHeight/2;
	int lv_iDstUWidth = lv_iDstHalfWidth;
	int lv_iDstVWidth = lv_iDstHalfWidth;

#if TEST_ENABLE_MEM_ENC_TEST==0
	int i,j;
	for( i=0; i<lv_iDstHeight; ++i)
	{
		memcpy( lv_pbtDst, lv_pbtSrc, lv_iDstWidth);
		lv_pbtDst += lv_iDstWidth;
		lv_pbtSrc += pm_pFrame->linesize[0];
	}

	unsigned char * lv_pbtSrcU = pm_pFrame->data[1];
	int lv_iUSkip = pm_pFrame->linesize[1] - lv_iDstUWidth;
	for( i=0; i<lv_iDstHalfHeight; ++i)
	{
		memcpy( lv_pbtDst, lv_pbtSrcU, lv_iDstUWidth);
		lv_pbtDst += lv_iDstUWidth;
		lv_pbtSrcU += pm_pFrame->linesize[1];
	}

	unsigned char * lv_pbtSrcV = pm_pFrame->data[2];
	int lv_iVSkip = pm_pFrame->linesize[2] - lv_iDstVWidth;
	for( i=0; i<lv_iDstHalfHeight; ++i)
	{
		memcpy( lv_pbtDst, lv_pbtSrcV, lv_iDstVWidth);
		lv_pbtDst += lv_iDstVWidth;
		lv_pbtSrcV += pm_pFrame->linesize[2];
	}
#endif

    SHWImageFrame sFrame;
    sFrame.pYBuf = &s->slv_pbtCacheImgBuf[0];
    sFrame.pUBuf = sFrame.pYBuf + lv_iDstWidth*lv_iDstHeight;
    sFrame.pVBuf = sFrame.pUBuf + lv_iDstUWidth*lv_iDstHalfHeight;

    sFrame.iYStride = lv_iDstWidth;
    sFrame.iUStride = lv_iDstUWidth;
    sFrame.iVStride = lv_iDstVWidth;

    sFrame.qPTS = pm_pFrame->pts;

    sFrame.iWidth	= lv_iDstWidth;
    sFrame.iHeight	= lv_iDstHeight;
    sFrame.iDataSize= sFrame.iYStride*sFrame.iHeight + sFrame.iUStride*lv_iDstHalfHeight + sFrame.iVStride*lv_iDstHalfHeight;

    *pm_pImgFrame = sFrame;
}


static void CopyNV12ToI420ImageFrame(SAndroidHwContext *s, SHWImageFrame * pm_pImgFrame, const AVFrame * pm_pFrame)
{
	unsigned char * lv_pbtDst = &s->slv_pbtCacheImgBuf[0];
	unsigned char * lv_pbtSrc = pm_pFrame->data[0];
	int lv_iDstWidth 	= pm_pFrame->width;
	int lv_iDstHeight 	= pm_pFrame->height;
	int lv_iDstHalfWidth = (lv_iDstWidth+1)/2;
	int lv_iDstHalfHeight= lv_iDstHeight/2;
	int i,j;
	for( i=0; i<lv_iDstHeight; ++i)
	{
		memcpy( lv_pbtDst, lv_pbtSrc, lv_iDstWidth);
		lv_pbtDst += lv_iDstWidth;
		lv_pbtSrc += pm_pFrame->linesize[0];
	}

	unsigned char * lv_pbtSrcUV = pm_pFrame->data[1];
	unsigned char * lv_pbtDstU	= lv_pbtDst;
	unsigned char * lv_pbtDstV	= lv_pbtDst + lv_iDstHalfWidth*lv_iDstHalfHeight;
	int lv_iUVSkip = pm_pFrame->linesize[1] - lv_iDstHalfWidth*2;
	int lv_iDstUVWidth = lv_iDstHalfWidth;

	for( i=0; i<lv_iDstHalfHeight; ++i)
	{
		for( j=0; j<lv_iDstHalfWidth; ++j)
		{
			lv_pbtDstU[0] = lv_pbtSrcUV[0];
			lv_pbtDstV[0] = lv_pbtSrcUV[1];
			++lv_pbtDstU;
			++lv_pbtDstV;
			lv_pbtSrcUV += 2;
		}
		lv_pbtSrcUV += lv_iUVSkip;
	}


    SHWImageFrame sFrame;
    sFrame.pYBuf = &s->slv_pbtCacheImgBuf[0];
    sFrame.pUBuf = sFrame.pYBuf + lv_iDstWidth*lv_iDstHeight;
    sFrame.pVBuf = sFrame.pUBuf + lv_iDstHalfWidth*lv_iDstHalfHeight;

    sFrame.iYStride = lv_iDstWidth;
    sFrame.iUStride = lv_iDstHalfWidth;
    sFrame.iVStride = lv_iDstHalfWidth;

    sFrame.qPTS = pm_pFrame->pts;

    sFrame.iWidth	= lv_iDstWidth;
    sFrame.iHeight	= lv_iDstHeight;
    sFrame.iDataSize= sFrame.iYStride*sFrame.iHeight + sFrame.iUStride*lv_iDstHalfHeight + sFrame.iVStride*lv_iDstHalfHeight;

    *pm_pImgFrame = sFrame;
}

static void CopyNV21ToI420pImageFrame( SAndroidHwContext *s, SHWImageFrame * pm_pImgFrame, const AVFrame * pm_pFrame)
{
	unsigned char * lv_pbtDst = &s->slv_pbtCacheImgBuf[0];
	unsigned char * lv_pbtSrc = pm_pFrame->data[0];
	int lv_iDstWidth 	= pm_pFrame->width;
	int lv_iDstHeight 	= pm_pFrame->height;
	int lv_iDstHalfWidth = (lv_iDstWidth+1)/2;
	int lv_iDstHalfHeight= lv_iDstHeight/2;
	int i,j;
	for( i=0; i<lv_iDstHeight; ++i)
	{
		memcpy( lv_pbtDst, lv_pbtSrc, lv_iDstWidth);
		lv_pbtDst += lv_iDstWidth;
		lv_pbtSrc += pm_pFrame->linesize[0];
	}

	unsigned char * lv_pbtSrcUV = pm_pFrame->data[1];
	unsigned char * lv_pbtDstU	= lv_pbtDst;
	unsigned char * lv_pbtDstV	= lv_pbtDst + lv_iDstHalfWidth*lv_iDstHalfHeight;
	int lv_iUVSkip = pm_pFrame->linesize[1] - lv_iDstHalfWidth*2;
	int lv_iDstUVWidth = lv_iDstHalfWidth;

	for( i=0; i<lv_iDstHalfHeight; ++i)
	{
		for( j=0; j<lv_iDstHalfWidth; ++j)
		{
			lv_pbtDstU[0] = lv_pbtSrcUV[1];
			lv_pbtDstV[0] = lv_pbtSrcUV[0];
			++lv_pbtDstU;
			++lv_pbtDstV;
			lv_pbtSrcUV += 2;
		}
		lv_pbtSrcUV += lv_iUVSkip;
	}


    SHWImageFrame sFrame;
    sFrame.pYBuf = &s->slv_pbtCacheImgBuf[0];
    sFrame.pUBuf = sFrame.pYBuf + lv_iDstWidth*lv_iDstHeight;
    sFrame.pVBuf = sFrame.pUBuf + lv_iDstHalfWidth*lv_iDstHalfHeight;

    sFrame.iYStride = lv_iDstWidth;
    sFrame.iUStride = lv_iDstHalfWidth;
    sFrame.iVStride = lv_iDstHalfWidth;

    sFrame.qPTS = pm_pFrame->pts;

    sFrame.iWidth	= lv_iDstWidth;
    sFrame.iHeight	= lv_iDstHeight;
    sFrame.iDataSize= sFrame.iYStride*sFrame.iHeight + sFrame.iUStride*lv_iDstHalfHeight + sFrame.iVStride*lv_iDstHalfHeight;

    *pm_pImgFrame = sFrame;
}

static void CopyTextureIDToImageFrame( SAndroidHwContext *s, SHWImageFrame * pm_pImgFrame, const AVFrame * pm_pFrame)
{
	YXTextureFrameST * lv_pbtDst = (YXTextureFrameST*)&s->slv_pbtCacheImgBuf[0];
	YXTextureFrameST * pTexFrame = (YXTextureFrameST*)pm_pFrame->opaque;
	if( NULL != pTexFrame)
	{
		*lv_pbtDst = *pTexFrame;
	}

    SHWImageFrame sFrame;
    sFrame.pYBuf = &s->slv_pbtCacheImgBuf[0];
    sFrame.pUBuf = NULL;
    sFrame.pVBuf = NULL;

    sFrame.iYStride = sizeof(YXTextureFrameST)/sizeof(int);
    sFrame.iUStride = 0;
    sFrame.iVStride = 0;

    sFrame.qPTS = pm_pFrame->pts;

    sFrame.iWidth	= pm_pFrame->width;
    sFrame.iHeight	= pm_pFrame->height;
    sFrame.iDataSize= sizeof(YXTextureFrameST);

    *pm_pImgFrame = sFrame;
#if USR_CODEC_DEBUG
//	YXLOGE("Texture id:[%d]", pTexFrame->i64TextureID);
#endif

}

static void CopyAVFrameToMyFrame( SAndroidHwContext *s, const AVFrame * _pFrame, int _srcFormat, int _dstFormat, SHWImageFrame * _pImgFrame)
{
	switch( _srcFormat)
	{
	case AV_PIX_FMT_YUV420P:
		if( COLOR_FormatYUV420Planar==_dstFormat)
		{
			CopyI420ToI420ImageFrame( s, _pImgFrame, _pFrame);
		}
		else if( 21==_dstFormat)
		{
			CopyI420FrameToImageFrame( s, _pImgFrame, _pFrame);
		}
		break;
	case AV_PIX_FMT_NV12:
		if( COLOR_FormatYUV420Planar==_dstFormat)
		{
			CopyNV12ToI420ImageFrame( s, _pImgFrame, _pFrame);
		}
		else if( COLOR_FormatYUV420SemiPlanar==_dstFormat)
		{
			CopyNV12FrameToImageFrame( s, _pImgFrame, _pFrame);
		}

		break;
	case AV_PIX_FMT_NV21:
		if( COLOR_FormatYUV420Planar==_dstFormat)
		{
			CopyNV21ToI420pImageFrame(s,  _pImgFrame, _pFrame);
		}
		else if( COLOR_FormatYUV420SemiPlanar==_dstFormat)
		{
			CopyNV21FrameToImageFrame( s, _pImgFrame, _pFrame);
		}
		break;
	case AV_PIX_FMT_VIDEOTOOLBOX:
		CopyTextureIDToImageFrame( s, _pImgFrame, _pFrame);
		break;
	default:
		break;
	}
}


AVCodec ff_android_hw_h264_encoder = {
    .name           = "h264_mediacodec_encoder",
    .long_name      = "Android H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_H264,
    .priv_data_size = sizeof(SAndroidHwContext),
    .init           = android_hw_encode_init,
    .encode2        = android_hw_encode_frame,
//	.send_frame		= android_hw_send_frame,
//	.receive_packet  = android_hw_recive_packet,
    .close          = android_hw_encode_close,
	.capabilities   = AV_CODEC_CAP_DELAY,
    .pix_fmts       = (const enum AVPixelFormat[]){ AV_PIX_FMT_NV12,
    												AV_PIX_FMT_NV21,
    												AV_PIX_FMT_YUV420P,
    												AV_PIX_FMT_VIDEOTOOLBOX,
                                                    AV_PIX_FMT_NONE },
    .priv_class     = &class,
};
#if 0
AVCodec ff_android_hw_h265_encoder = {
		.name           = "h265_mediacodec_encoder",
		.long_name      = "Android H.265 / HEVC / MPEG-4 HEVC",
		.type           = AVMEDIA_TYPE_VIDEO,
		.id             = AV_CODEC_ID_HEVC,
		.priv_data_size = sizeof(SAndroidHwContext),
		.init           = android_hw_encode_init_hevc,
		.encode2        = android_hw_encode_frame,
		.send_frame		= android_hw_send_frame,
		.receive_packet  = android_hw_recive_packet,
		.close          = android_hw_encode_close,
		.capabilities   = CODEC_CAP_DELAY,
		.pix_fmts       = (const enum AVPixelFormat[]){
														AV_PIX_FMT_VIDEOTOOLBOX,
														AV_PIX_FMT_NONE },
		.priv_class     = &class,
};
#endif