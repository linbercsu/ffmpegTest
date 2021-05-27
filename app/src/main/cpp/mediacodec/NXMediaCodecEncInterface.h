/*
 * com_nxinc_mediacodec_interface.h
 *
 *  Created on: 2016-7-27
 *      Author:
 */

#ifndef COM_NXINC_MEDIACODEC_INTERFACE_H_
#define COM_NXINC_MEDIACODEC_INTERFACE_H_
#include <stdint.h>
#include <stdbool.h>
#include <jni.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct YX_AMediaCodec_Enc_Opaque       YX_AMediaCodec_Enc_Opaque;
typedef struct YX_AMediaCodec_Enc              YX_AMediaCodec_Enc;

struct YX_AMediaCodec_Enc
{
	YX_AMediaCodec_Enc_Opaque * opaque;

};

int		YX_AMediaCodec_Enc_loadClassEnv( JavaVM * _pGloabJvm, int _jniVersion);

bool 	YX_AMediaCodec_Enc_isInNotSupportedList( );

YX_AMediaCodec_Enc * YX_AMediaCodec_Enc_createEncoderObject();

void 	YX_AMediaCodec_Enc_destoryEncoderObject( YX_AMediaCodec_Enc * _ctx);

int 	YX_AMediaCodec_Enc_initEncoder( YX_AMediaCodec_Enc * _ctx, int _width, int _height, int _frameRate, int _colorFormat, int _iFrameInterval, int _bitRate, int _profile, bool _useInputSurface, int _encType);

int 	YX_AMediaCodec_Enc_encodeVideoFromBuffer( YX_AMediaCodec_Enc * _ctx, uint8_t * _input, int _inputSize, uint8_t * _output, int _maxOutputSize, int * _pOutSize,  int ouputFlag, int64_t pts);

int 	YX_AMediaCodec_Enc_encodeVideoFromTexture( YX_AMediaCodec_Enc * _ctx, int * _input, int _inputSize, uint8_t * _output, int _maxOutputSize, int * _pOutSize,  int ouputFlag);

int 	YX_AMediaCodec_Enc_getLastFrameFlags( YX_AMediaCodec_Enc * _ctx);

int 	YX_AMediaCodec_Enc_closeEncoder( YX_AMediaCodec_Enc * _ctx);

int 	YX_AMediaCodec_Enc_getSupportedColorFormat( YX_AMediaCodec_Enc * _ctx);

void 	YX_AMediaCodec_Enc_refreshExtraData( YX_AMediaCodec_Enc * _ctx);

int		YX_AMediaCodec_Enc_getExtraDataSize( YX_AMediaCodec_Enc * _ctx);

void 	YX_AMediaCodec_Enc_getExtraData( YX_AMediaCodec_Enc * _ctx, uint8_t * _extraData, int _maxSize);

void	YX_AMediaCodec_Enc_setEncoder( YX_AMediaCodec_Enc * _ctx, int _width, int _height, int _frameRate, int _bitRate, int _iFrameInterval, int _colorFormat,  int _profile);

int		YX_AMediaCodec_Enc_getColorFormat( YX_AMediaCodec_Enc * _ctx);

int64_t	YX_AMediaCodec_Enc_getLastCodecPts( YX_AMediaCodec_Enc * _ctx);

uint32_t findStartCode(uint8_t *in_pBuffer, uint32_t in_ui32BufferSize, uint32_t in_ui32Code,
					   uint32_t * out_ui32ProcessedBytes);

#ifdef __cplusplus
}
#endif

#endif /* COM_NXINC_MEDIACODEC_INTERFACE_H_ */
