//
//  on 2017/6/21.
//


#include <stdint.h>
#include <string.h>
#include <android/JniUtils/NXAndroidJniBase.h>

#include "libavutil/avassert.h"
#include "libavutil/fifo.h"
#include "libavcodec/avcodec.h"
#include "libavutil/internal.h"
#include "libavcodec/internal.h"
#include "NXMediaCodecDecJni.h"
#include "NXUtilCodecInfoProcess.h"


#define USR_DEF_MAX_PATH 1024

#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif
#ifndef INOUT
#define INOUT
#endif

typedef struct SYXMediaCodecH264DecContext {

    SYXMediaCodecDecContext * ctx;
    AVBitStreamFilterContext * bsf;
    AVFifoBuffer * fifo;

} SYXMediaCodecH264DecContext;


static av_cold int mediacodec_decode_close(AVCodecContext *avctx)
{
    SYXMediaCodecH264DecContext *s = avctx->priv_data;
    YXLOGE("call %s %d", __FUNCTION__, __LINE__);

    YX_MediaCodecDec_close( &s->ctx);

    av_fifo_free(s->fifo);

    av_bitstream_filter_close(s->bsf);

    return 0;
}



static int h264_ps_to_nalu(const uint8_t *src, int src_size, uint8_t **out, int *out_size)
{
    int i;
    int ret = 0;
    uint8_t *p = NULL;
    static const uint8_t nalu_header[] = { 0x00, 0x00, 0x00, 0x01 };

    if (!out || !out_size) {
        return AVERROR(EINVAL);
    }

    p = (uint8_t *) av_malloc(sizeof(nalu_header) + src_size);
    if (!p) {
        return AVERROR(ENOMEM);
    }

    *out = p;
    *out_size = sizeof(nalu_header) + src_size;

    memcpy(p, nalu_header, sizeof(nalu_header));
    memcpy(p + sizeof(nalu_header), src, (size_t)src_size);

    /* Escape 0x00, 0x00, 0x0{0-3} pattern */
    for (i = 4; i < *out_size; i++) {
        if (i < *out_size - 3 &&
            p[i + 0] == 0 &&
            p[i + 1] == 0 &&
            p[i + 2] <= 3) {
            uint8_t *newData;

            *out_size += 1;
            newData = (uint8_t *) av_realloc(*out, (size_t)(*out_size));
            if (!newData) {
                ret = AVERROR(ENOMEM);
                goto done;
            }
            *out = p = newData;

            i = i + 3;
            memmove(p + i, p + i - 1, (size_t)(*out_size - i));
            p[i - 1] = 0x03;
        }
    }
    done:
    if (ret < 0) {
        av_freep(out);
        *out_size = 0;
    }

    return ret;
}


static int ps_process( uint8_t * INOUT _pps, int * INOUT _ppsSize, uint8_t * INOUT _sps, int * INOUT _spsSize)
{
    uint8_t *data = NULL;
    int data_size = 0;
    int ret = -1;

    if ((ret = h264_ps_to_nalu(_pps, *_ppsSize, &data, &data_size)) < 0) {
        goto done;
    }
    memcpy( _pps, data, (size_t)data_size);
    *_ppsSize = data_size;
    av_freep(&data);

    if ((ret = h264_ps_to_nalu(_sps, *_spsSize, &data, &data_size)) < 0) {
        goto done;
    }
    memcpy( _sps, data, (size_t)data_size);
    *_ppsSize = data_size;
    av_freep(&data);

    done:

    return ret;

}

static unsigned char s_pBufSPS[USR_DEF_MAX_PATH] = {0};
static unsigned char s_pBufPPS[USR_DEF_MAX_PATH] = {0};

static av_cold int mediacodec_decode_init(AVCodecContext *avctx)
{
    int ret, spsSize, ppsSize;
    uint8_t* dummy = NULL;
    int dummy_len;


    SYXMediaCodecH264DecContext *s = avctx->priv_data;
    YXLOGE("call %s %d", __FUNCTION__, __LINE__);

    s->bsf = av_bitstream_filter_init("h264_mp4toannexb");
    if(!s->bsf) {
        ret = AVERROR_BSF_NOT_FOUND;
        goto done;
    }

    av_bitstream_filter_filter(s->bsf, avctx, NULL, &dummy, &dummy_len, NULL, 0, 0);
    if ( NULL != dummy)
    {
        av_free(dummy);
        dummy = NULL;
    }

#if 1

#if 1
    if ( avctx->extradata[0]==0
         && avctx->extradata[1]==0
         && avctx->extradata[2]==0
         && avctx->extradata[3]==1)
    {
        memcpy( s_pBufSPS, avctx->extradata, avctx->extradata_size);
        spsSize = avctx->extradata_size;
        ppsSize = 0;
        ret = 0;

    } else
    {
        ret = YX_H264_Decode_extradata( avctx->extradata, avctx->extradata_size,
                                        s_pBufPPS, &ppsSize,
                                        s_pBufSPS, &spsSize);

    }
    if ( ret < 0)
    {
        // TODO:兼容从流里面获取extradata；
        // ...
        // else
        // ...
        goto done;
    }

//    ps_process( s_pBufPPS, &ppsSize, s_pBufSPS, &spsSize);

    ret = YX_MeidaCodecDec_init( &s->ctx,
                                 avctx->width, avctx->height,
                                 s_pBufSPS, spsSize,
                                 s_pBufPPS, ppsSize);
#else

    YX_H264_Decode_extradata_ex( (uint8_t*) avctx->extradata,
                                 (uint32_t) avctx->extradata_size,
                                 s_pBufPPS, &ppsSize,
                                 s_pBufSPS, &spsSize);



    ret = YX_MeidaCodecDec_init( &s->ctx,
                                 avctx->width, avctx->height,
                                 s_pBufSPS, spsSize,
                                 s_pBufPPS, ppsSize);

#endif

#else
    memcpy( s_pBufSPS, avctx->extradata, avctx->extradata_size);
    spsSize = avctx->extradata_size;
    ppsSize = 0;


    ret = YX_MeidaCodecDec_init( &s->ctx,
                                 avctx->width, avctx->height,
                                 s_pBufSPS, spsSize,
                                 s_pBufPPS, ppsSize);

#endif

    if ( ret < 0)
    {
        goto done;
    }

    av_log(avctx, AV_LOG_INFO, "MediaCodec started successfully, ret = %d\n", ret);

    s->fifo = av_fifo_alloc(sizeof(AVPacket));
    if (!s->fifo) {
        ret = AVERROR(ENOMEM);
        goto done;
    }

    done:

    if (ret < 0) {
        mediacodec_decode_close(avctx);
    }



    return ret;
}

typedef struct YXTextureFrameST
{
    int iTextureID;
    int iSharedEGLContext;

}YXTextureFrameST;


static int mediacodec_process_data(AVCodecContext *avctx, AVFrame *frame,
                                   int *got_frame, AVPacket *pkt)
{
    SYXMediaCodecH264DecContext *s = avctx->priv_data;
    int ret = -1;
    YXTextureFrameST dstTex = {0};
    uint8_t * pData = NULL;
    int iDataSize = 0;
    if ( NULL != pkt)
    {
        pData = pkt->data;
        iDataSize = pkt->size;
    }

    memcpy( &dstTex, avctx->opaque, sizeof(dstTex));

    ret = YX_MediaCodecDec_decode_frame( s->ctx, pData, iDataSize, pkt->pts, dstTex.iTextureID);

    if ( ret < 0)
    {
        *got_frame = 0;
        goto done;
    }
    *got_frame = 1;
    frame->width = avctx->width;
    frame->height = avctx->height;
    frame->format = AV_PIX_FMT_VIDEOTOOLBOX;
    frame->pkt_pts = YX_MediaCodec_get_info_by_flag( s->ctx, 2);
    frame->pkt_dts = AV_NOPTS_VALUE;

    done:
    return pkt->size;
}

static void printBytes( uint8_t * _pData, int _iCount)
{
    char printfBuf[2048] = {0};
    int line = _iCount/4;
    int i;
    uint8_t  * pData = _pData;
    for ( i = 0; i < line; ++i) {
        sprintf( printfBuf + strlen(printfBuf), "[%02x, %02x, %02x, %02x]\n", pData[0], pData[1], pData[2], pData[3]);
        pData+=4;
    }
    YXLOGE( "\n%s", printfBuf);
}

static int mediacodec_decode_frame(AVCodecContext *avctx, void *data,
                                   int *got_frame, AVPacket *avpkt)
{
    SYXMediaCodecH264DecContext *s = avctx->priv_data;
    AVFrame *frame    = data;
    int ret;

    /* buffer the input packet */
    if (avpkt->size) {
        AVPacket input_pkt = { 0 };

        if (av_fifo_space(s->fifo) < sizeof(input_pkt)) {
            ret = av_fifo_realloc2(s->fifo,
                                   av_fifo_size(s->fifo) + sizeof(input_pkt));
            if (ret < 0)
                return ret;
        }

        ret = av_packet_ref(&input_pkt, avpkt);
        if (ret < 0)
            return ret;
        av_fifo_generic_write(s->fifo, &input_pkt, sizeof(input_pkt), NULL);
    }

    /* process buffered data */
    while (!*got_frame) {
        /* prepare the input data -- convert to Annex B if needed */
        if (1) {
            AVPacket input_pkt = { 0 };

            /* no more data */
            if (av_fifo_size(s->fifo) < sizeof(AVPacket)) {
                return avpkt->size ? avpkt->size :
                       mediacodec_process_data(avctx, frame, got_frame, avpkt);
            }

            av_fifo_generic_read(s->fifo, &input_pkt, sizeof(input_pkt), NULL);

            {
                AVPacket fpkt = input_pkt;
                uint8_t * pData = NULL;
                int pSize = 0;
                int a = av_bitstream_filter_filter(s->bsf,
                                                   avctx, NULL, &pData, &pSize,
                                                   input_pkt.data, input_pkt.size, input_pkt.flags & AV_PKT_FLAG_KEY);

                input_pkt.data = pData;
                input_pkt.size = pSize;

                if ( a > 0) {
                    ret = mediacodec_process_data(avctx, frame, got_frame, &input_pkt);
                    av_freep( &pData);
                }
                else if( a==0)
                {
                    ret = mediacodec_process_data(avctx, frame, got_frame, &input_pkt);
                }

                input_pkt.data = fpkt.data;
                input_pkt.size = fpkt.size;

                av_packet_unref( &input_pkt);
            }

            return ret;
        }

    }
    YXLOGE( "call %s %d ", __FUNCTION__, __LINE__);

    return avpkt->size;
}


void convertPacket(AVPacket * packet) {
    uint8_t* data = 0;
    int pos = 0;
    long sum = 0;
    uint8_t header[4];
    header[0] = 0;
    header[1] = 0;
    header[2] = 0;
    header[3] = 1;
    while (pos < packet->size) {
        data = packet->data+pos;
        sum = data[0]*16777216+data[1]*65536+data[2]*256+data[3];
        if( sum < 2)
        {
            break;
        }
        memcpy(data, header, 4);
        pos += (int)sum;
        pos += 4;
    }
}

static int mediacodec_decode_frame2(AVCodecContext *avctx, void *data,
                                   int *got_frame, AVPacket *avpkt)
{
    SYXMediaCodecH264DecContext *s = avctx->priv_data;
    AVFrame *frame    = data;
    int ret;

    /* buffer the input packet */
    if (avpkt->size) {
        AVPacket input_pkt = { 0 };

        if (av_fifo_space(s->fifo) < sizeof(input_pkt)) {
            ret = av_fifo_realloc2(s->fifo,
                                   av_fifo_size(s->fifo) + sizeof(input_pkt));
            if (ret < 0)
                return ret;
        }

        ret = av_packet_ref(&input_pkt, avpkt);
        if (ret < 0)
            return ret;
        av_fifo_generic_write(s->fifo, &input_pkt, sizeof(input_pkt), NULL);
    }

    /* process buffered data */
    while (!*got_frame) {
        /* prepare the input data -- convert to Annex B if needed */
        if (1) {
            AVPacket input_pkt = { 0 };

            /* no more data */
            if (av_fifo_size(s->fifo) < sizeof(AVPacket)) {
                return avpkt->size ? avpkt->size :
                       mediacodec_process_data(avctx, frame, got_frame, avpkt);
            }

            av_fifo_generic_read(s->fifo, &input_pkt, sizeof(input_pkt), NULL);

            {
                convertPacket(&input_pkt);
//                if ( input_pkt.flags & AV_PKT_FLAG_KEY)
//                {
//                    printBytes( input_pkt.data, 60);
//                }
                ret = mediacodec_process_data(avctx, frame, got_frame, &input_pkt);
                av_packet_unref( &input_pkt);
            }

            return ret;
        }

    }
    YXLOGE( "call %s %d ", __FUNCTION__, __LINE__);

}

static int mediacodec_decode_frame3(AVCodecContext *avctx, void *data,
                                    int *got_frame, AVPacket *avpkt)
{
    AVFrame *frame    = data;
    int ret;
    convertPacket(avpkt);
    ret = mediacodec_process_data(avctx, frame, got_frame, avpkt);

    return  ret;
}


static int mediacodec_send_packet( AVCodecContext *avctx, const AVPacket *avpkt)
{
    SYXMediaCodecH264DecContext *s = avctx->priv_data;
    int ret;
//    YXLOGE( "call %s %d ", __FUNCTION__, __LINE__);
    if ( avpkt != NULL && avpkt->size) {
        AVPacket input_pkt = { 0 };

        if (av_fifo_space(s->fifo) < sizeof(input_pkt)) {
            ret = av_fifo_realloc2(s->fifo,
                                   av_fifo_size(s->fifo) + sizeof(input_pkt));
            if (ret < 0)
                return ret;
        }

        ret = av_packet_ref(&input_pkt, avpkt);
        if (ret < 0)
            return ret;
        av_fifo_generic_write(s->fifo, &input_pkt, sizeof(input_pkt), NULL);
    } else
    {

    }

    return 0;
}

static int mediacodec_receive_frame( AVCodecContext *avctx, AVFrame *data)
{
    SYXMediaCodecH264DecContext *s = avctx->priv_data;
    AVFrame *frame    = data;
    int ret = AVERROR(EAGAIN);
    int got = 0;
    int * got_frame = &got;
    /* process buffered data */
    while (!*got_frame)
    {
        AVPacket input_pkt = { 0 };

        /* no more data */
        if (av_fifo_size(s->fifo) < sizeof(AVPacket))
        {
            return AVERROR(EAGAIN);
        }

        av_fifo_generic_read(s->fifo, &input_pkt, sizeof(input_pkt), NULL);
        convertPacket(&input_pkt);

        ret = mediacodec_process_data( avctx, frame, got_frame, &input_pkt);
        if ( *got_frame)
        {
            ret = 0;
        }
        av_packet_unref( &input_pkt);

    }

    return ret;
}


static void mediacodec_decode_flush(AVCodecContext *avctx)
{
    SYXMediaCodecH264DecContext *s = avctx->priv_data;

    YXLOGE("call %s %d", __FUNCTION__, __LINE__);
    while (av_fifo_size(s->fifo)) {
        AVPacket pkt;
        av_fifo_generic_read(s->fifo, &pkt, sizeof(pkt), NULL);
        av_packet_unref(&pkt);
    }
    av_fifo_reset(s->fifo);

    YX_MediaCodecDec_flush(s->ctx);
}

AVCodec ff_android_hw_h264_decoder = {
        .name           = "h264_mediacodec_decoder",
        .long_name      = NULL_IF_CONFIG_SMALL("H.264 Android MediaCodec decoder"),
        .type           = AVMEDIA_TYPE_VIDEO,
        .id             = AV_CODEC_ID_H264,
        .priv_data_size = sizeof(SYXMediaCodecH264DecContext),
        .init           = mediacodec_decode_init,
        .decode         = mediacodec_decode_frame2,
        .send_packet    = mediacodec_send_packet,
        .receive_frame  = mediacodec_receive_frame,
        .flush          = mediacodec_decode_flush,
        .close          = mediacodec_decode_close,
        .capabilities   = CODEC_CAP_DELAY,
        .caps_internal  = FF_CODEC_CAP_INIT_THREADSAFE,
};
