//
//  on 2017/1/9.
//

#ifndef VIDEOEDIT_NXUTILCODECINFOPROCESS_H
#define VIDEOEDIT_NXUTILCODECINFOPROCESS_H
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * 将关键数据转换为mp4使用的avcC格式;
 */
void processExtraData2AVCC(uint8_t *_pExtraData, int *inout_iExtraSize);

/**
 * 将关键数据转换为mp4使用的HVCC格式;
 */
void processExtraData2HVCC(uint8_t *_pExtraData, int *inout_iExtraSize, void* logCtx);

/**
 * 将数据中对应的00 00 00 01修改为对应数据长度;
 */
void convertH2645ExtraDataFlagToSize(uint8_t * _pBuffer, int _size, int isHEVC);


void YX_StreamParser_init();

void YX_StreamParser_release();

void YX_StreamParser_analysis( void * _pInData, int _iInSize, void ** _ppOut, int * _pOutSize);

void YX_Dump_Hex( void * _pData, int _size, int _step);

int YX_H264_Decode_extradata( void * _pInData, int _iInSize,
                               unsigned char * _pPPS, int * _iPPSSize,
                               unsigned char * _pSPS, int * _iSPSSIze
                                );

int YX_H264_Decode_extradata_ex( void * _pInData, int _iInSize,
                                 unsigned char * _pPPS, int * _iPPSSize,
                                 unsigned char * _pSPS, int * _iSPSSIze
);



#ifdef __cplusplus
};
#endif

#endif //VIDEOEDIT_NXUTILCODECINFOPROCESS_H
