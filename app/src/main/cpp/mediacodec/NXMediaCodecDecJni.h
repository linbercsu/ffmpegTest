//
//  on 2017/6/21.
//

#ifndef VIDEOEDITFACEDECTECT_YXMEDIACODECDECJNI_H
#define VIDEOEDITFACEDECTECT_YXMEDIACODECDECJNI_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct SYXMediaCodecDecContext
{
    void * mediacodecCtx;

}SYXMediaCodecDecContext;


bool YXMediaCodecDecJniLoadClass( void * _pGloabJvm, int _jniVersion);


int YX_MeidaCodecDec_init( SYXMediaCodecDecContext ** _ctx,
                           int _width, int _height,
                           unsigned char* sps, int spsSize,
                           unsigned char* pps, int ppsSize
);

int YX_MediaCodecDec_set( SYXMediaCodecDecContext * _ctx,
                          int _width, int _height,
                          unsigned char* sps, int spsSize,
                          unsigned char* pps, int ppsSize
);

int YX_MediaCodecDec_close( SYXMediaCodecDecContext ** _ctx);

int YX_MediaCodecDec_decode_frame( SYXMediaCodecDecContext * _ctx,
                                   unsigned char*  _frameData, int _inputSize,
                                   long long _timeStamp, int _outputTex
);

long long YX_MediaCodec_get_info_by_flag( SYXMediaCodecDecContext * _ctx, int _flag);

int YX_MediaCodecDec_flush(SYXMediaCodecDecContext * _ctx);

#ifdef __cplusplus
};
#endif

#endif //VIDEOEDITFACEDECTECT_YXMEDIACODECDECJNI_H
