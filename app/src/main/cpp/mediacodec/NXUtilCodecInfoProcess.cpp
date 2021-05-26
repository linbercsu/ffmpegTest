//
//  on 2017/1/9.
//


#include "NXUtilCodecInfoProcess.h"
#include "android/JniUtils/NXAndroidJniBase.h"
//#include "yx_hevc_parse.h"

#define LOG_TAG "YXUtilCodecInfoProcess"

// android关键数据转换未ffmpeg关键数据;
#define is_start_code(code)	(((code) & 0x0ffffff) == 0x01)

#define USR_DEF_MAX_CACHE_BUF_SIZE 1024*1024

#define H264_NALU_TYPE_NON_IDR_PICTURE								1
#define H264_NALU_TYPE_IDR_PICTURE									5
#define H264_NALU_TYPE_SEI                                          6
#define H264_NALU_TYPE_SEQUENCE_PARAMETER_SET						7
#define H264_NALU_TYPE_PICTURE_PARAMETER_SET						8

#define H265_NALU_TYPE_NON_IDR_PICTURE_MIN							0
#define H265_NALU_TYPE_NON_IDR_PICTURE_MAX							9
#define H265_NALU_TYPE_IDR_PICTURE_MIN							    16
#define H265_NALU_TYPE_IDR_PICTURE_MAX								21
#define H265_NALU_TYPE_VPS						                    32
#define H265_NALU_TYPE_SPS						                    33
#define H265_NALU_TYPE_PPS						                    34
#define H265_NALU_TYPE_SEI                                          39


#define USR_DEF_PRINT_LOG 0

static unsigned char sv_pbtSwapImgBuf[USR_DEF_MAX_CACHE_BUF_SIZE] = {0};


static uint32_t findStartCode(uint8_t *in_pBuffer, uint32_t in_ui32BufferSize, uint32_t in_ui32Code,
                              uint32_t * out_ui32ProcessedBytes) {
    uint32_t ui32Code = in_ui32Code;

    uint8_t *ptr = in_pBuffer;
    while (ptr < in_pBuffer + in_ui32BufferSize) {
        ui32Code = *ptr++ + (ui32Code << 8);
        if (is_start_code(ui32Code))
            break;
    }

    *out_ui32ProcessedBytes = (uint32_t)(ptr - in_pBuffer);

    return ui32Code;
}

void convertH2645ExtraDataFlagToSize(uint8_t * _pBuffer, int _size, int isHEVC)
//void convertExtraDataFlagToSize(uint8_t * _pBuffer, int _size)
{
    uint32_t ui32StartCode = 0x00;

    uint8_t * pBuffer = _pBuffer;
    int  ui32BufferSize = _size;

    int prePos = -1;
    int curPos = 0;

    do {
        uint32_t ui32ProcessedBytes = 0;
        ui32StartCode = findStartCode(pBuffer, ui32BufferSize, ui32StartCode, &ui32ProcessedBytes);
        pBuffer += ui32ProcessedBytes;
        ui32BufferSize -= ui32ProcessedBytes;

        if (ui32BufferSize < 1)
            break;

        uint8_t val;
        int isValidPacket;
        if ( isHEVC)
        {
            val = (uint8_t)((*pBuffer & 0x7e)>>1);
            isValidPacket = 1;/*(
                    val == H265_NALU_TYPE_VPS
                    || val == H265_NALU_TYPE_SPS
                    || val == H265_NALU_TYPE_PPS
                    || val == H265_NALU_TYPE_SEI
                    || (val >= H265_NALU_TYPE_NON_IDR_PICTURE_MIN && val <= H265_NALU_TYPE_NON_IDR_PICTURE_MAX)
                    || (val >= H265_NALU_TYPE_IDR_PICTURE_MIN && val <= H265_NALU_TYPE_IDR_PICTURE_MAX)
            );*/
        }
        else
        {

            val = (uint8_t)(*pBuffer & 0x1f);
            isValidPacket = (
                    val == H264_NALU_TYPE_NON_IDR_PICTURE
                    || val == H264_NALU_TYPE_IDR_PICTURE
                    || val == H264_NALU_TYPE_SEI
                    || val == H264_NALU_TYPE_SEQUENCE_PARAMETER_SET
                    || val == H264_NALU_TYPE_PICTURE_PARAMETER_SET
            );
        }
#if USR_DEF_PRINT_LOG
        YXLOGE( "call %s %d pos[%u] val=[%u]", __FUNCTION__, __LINE__, ui32ProcessedBytes, val);
#endif

        if ( isValidPacket)
        {
            curPos += ui32ProcessedBytes;
            if ( prePos > 0)
            {
                int preSize = curPos - prePos - 4;
                uint8_t * preData = _pBuffer + prePos - 4;
                preData[0] = (uint8_t)(((preSize) >> 24) & 0x00ff);
                preData[1] = (uint8_t)(((preSize) >> 16) & 0x00ff);
                preData[2] = (uint8_t)(((preSize) >> 8) & 0x00ff);
                preData[3] = (uint8_t)(((preSize)) & 0x00ff);
#if USR_DEF_PRINT_LOG
                YXLOGE( "xxxxxxxx pos:[%d]", prePos-4);
#endif

            }
            prePos = curPos;
        }

    } while (ui32BufferSize > 0);

    if ( prePos > 0)
    {
        int preSize = _size - prePos;
        uint8_t * preData = _pBuffer + prePos - 4;
        preData[0] = (uint8_t)(((preSize) >> 24) & 0x00ff);
        preData[1] = (uint8_t)(((preSize) >> 16) & 0x00ff);
        preData[2] = (uint8_t)(((preSize) >> 8) & 0x00ff);
        preData[3] = (uint8_t)(((preSize)) & 0x00ff);
#if USR_DEF_PRINT_LOG
        YXLOGE( "xxxxxxxx pos:[%d]", prePos-4);
#endif
    }

}

static void parseH264SequenceHeader(uint8_t *in_pBuffer, uint32_t in_ui32Size,
                                    uint8_t **inout_pBufferSPS, int *inout_ui32SizeSPS,
                                    uint8_t **inout_pBufferPPS, int *inout_ui32SizePPS)
{
    uint32_t ui32StartCode = 0x0ff;

    uint8_t * pBuffer = in_pBuffer;
    uint32_t  ui32BufferSize = in_ui32Size;

    uint32_t sps = 0;
    uint32_t pps = 0;

    uint32_t idr = in_ui32Size;

    do {
        uint32_t ui32ProcessedBytes = 0;
        ui32StartCode = findStartCode(pBuffer, ui32BufferSize, ui32StartCode, &ui32ProcessedBytes);
        pBuffer += ui32ProcessedBytes;
        ui32BufferSize -= ui32ProcessedBytes;

        if (ui32BufferSize < 1)
            break;

        uint8_t val = (uint8_t)(*pBuffer & 0x1f);

        if (val == 5)
            idr = pps + ui32ProcessedBytes - 4;

        if (val == 7)
            sps = ui32ProcessedBytes;

        if (val == 8)
            pps = sps + ui32ProcessedBytes;

    } while (ui32BufferSize > 0);

    *inout_pBufferSPS = in_pBuffer + sps - 4;
    *inout_ui32SizeSPS = pps - sps;

    *inout_pBufferPPS = in_pBuffer + pps - 4;
    *inout_ui32SizePPS = idr - pps + 4;
}


void processExtraData2AVCC(uint8_t *_pExtraData, int *inout_iExtraSize)
{
    uint8_t* spsFrame = 0;
    uint8_t* ppsFrame = 0;

    int spsFrameLen = 0;
    int ppsFrameLen = 0;

    uint8_t * resultBuf = &sv_pbtSwapImgBuf[0];
    int 	  resultSize = 0;

    parseH264SequenceHeader(_pExtraData, *inout_iExtraSize, &spsFrame, &spsFrameLen, &ppsFrame, &ppsFrameLen);

    int extradata_len = 8 + spsFrameLen - 4 + 1 + 2 + ppsFrameLen - 4;
    resultSize = extradata_len;
    resultBuf[0] = 0x01;
    resultBuf[1] = spsFrame[4 + 1];
    resultBuf[2] = spsFrame[4 + 2];
    resultBuf[3] = spsFrame[4 + 3];
    resultBuf[4] = 0xFC | 3;
    resultBuf[5] = 0xE0 | 1;
    int tmp = spsFrameLen - 4;
    resultBuf[6] = (uint8_t)((tmp >> 8) & 0x00ff);
    resultBuf[7] = (uint8_t)(tmp & 0x00ff);
    int i = 0;
    for (i = 0; i < tmp; i++)
        resultBuf[8 + i] = spsFrame[4 + i];
    resultBuf[8 + tmp] = 0x01;
    int tmp2 = ppsFrameLen - 4;
    resultBuf[8 + tmp + 1] = (uint8_t)((tmp2 >> 8) & 0x00ff);
    resultBuf[8 + tmp + 2] = (uint8_t)(tmp2 & 0x00ff);
    for (i = 0; i < tmp2; i++)
        resultBuf[8 + tmp + 3 + i] = ppsFrame[4 + i];

    *inout_iExtraSize = resultSize;
    memcpy( _pExtraData, resultBuf, resultSize);

}


extern "C"
{
#include "libavcodec/avcodec.h"
//#include "libavcodec/bytestream.h"
//#include "libavcodec/h2645_parse.h"
//#include "libavcodec/hevc_sei.h"
//#include "libavcodec/hevc_ps.h"
//#include "libavcodec/hevc.h"
}
#define USR_DEF_MAX_SIZE 1024*64
class NXAvcStreamParser {

public:
    NXAvcStreamParser()
            :m_pH264ParserCtx(NULL)
            ,m_pH264codecCtx(NULL)
            ,m_pLastData(NULL)
            ,m_iSize(0)
            ,m_iPos(0)
    {
        init();
    }

    ~NXAvcStreamParser()
    {
        release();
    }

public:
    int pushH264StreamData( void * _pInData, int _iInSize)
    {
        uint8_t * pOutData = NULL;
        int outSize = 0;
        uint8_t  * pInData = NULL;
        int inSize = 0;
        m_iSize = 0;
        if ( m_iPos==0)
        {
            memcpy( inputArray0, _pInData, _iInSize);
            pInData = inputArray0;
            inSize = _iInSize;
        }
        else if ( m_iPos==1)
        {
            memcpy( inputArray1, _pInData, _iInSize);
            pInData = inputArray1;
            inSize = _iInSize;

        }
        int len = av_parser_parse2(m_pH264ParserCtx, m_pH264codecCtx, &pOutData, &outSize, pInData, inSize, 0, 0, 0);
#if USR_DEF_PRINT_LOG
        YXLOGE( "call %s %d IN [%d] Use:[%d] out:[%d]", __FUNCTION__, __LINE__, _iInSize, len, outSize);
#endif

        m_iPos==0?1:0;

        if ( outSize > 0)
        {
            ensureCapacity( outSize);

            memcpy( m_pLastData, pOutData, outSize);
            m_iSize = outSize;
        }

        return outSize;
    }

    void * getLastData()
    {
        return m_pLastData;
    }

    int getLastDataLength()
    {
        return m_iSize;
    }

private:
    int init()
    {
        m_pH264ParserCtx = av_parser_init(AV_CODEC_ID_H264);
        AVCodec* pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
        m_pH264codecCtx = avcodec_alloc_context3(pCodec);
        ensureCapacity(40960);
        return 0;
    }

    void release()
    {
        if ( NULL != m_pH264ParserCtx)
        {
            av_parser_close( m_pH264ParserCtx);
            m_pH264ParserCtx = NULL;
        }

        if ( NULL != m_pH264codecCtx)
        {
            avcodec_close(m_pH264codecCtx);
            av_free(m_pH264codecCtx);
            m_pH264codecCtx = NULL;
        }

        if ( NULL != m_pLastData)
        {
            delete [] m_pLastData;
            m_pLastData = NULL;
        }
        m_iMaxSize = 0;

    }

    void ensureCapacity( int nSize)
    {
        if ( m_iMaxSize < nSize)
        {
            if ( NULL != m_pLastData)
            {
                delete  [] m_pLastData;
                m_pLastData = NULL;
            }

            m_iMaxSize = nSize;
            m_pLastData = new uint8_t[m_iMaxSize];
        }

    }

private:
    AVCodecParserContext * m_pH264ParserCtx;
    AVCodecContext* m_pH264codecCtx;
    uint8_t  * m_pLastData;
    int m_iSize;
    int m_iMaxSize;
    uint8_t  inputArray0[USR_DEF_MAX_SIZE];
    uint8_t  inputArray1[USR_DEF_MAX_SIZE];
    int m_iPos;
};

static NXAvcStreamParser * sv_pAvcStreamParser = NULL;

void YX_StreamParser_init()
{
    if ( NULL == sv_pAvcStreamParser)
    {
        sv_pAvcStreamParser = new NXAvcStreamParser();
    }
}

void YX_StreamParser_release()
{
    if ( NULL != sv_pAvcStreamParser)
    {
        delete sv_pAvcStreamParser;
        sv_pAvcStreamParser = NULL;
    }
}

void YX_StreamParser_analysis( void * _pInData, int _iInSize, void ** _ppOut, int * _pOutSize)
{
    if ( NULL != sv_pAvcStreamParser)
    {
        int ret = sv_pAvcStreamParser->pushH264StreamData( _pInData, _iInSize);
        *_pOutSize = 0;
        if ( ret > 0)
        {
            *_ppOut = sv_pAvcStreamParser->getLastData();
            *_pOutSize = sv_pAvcStreamParser->getLastDataLength();
        }
    }
}
#define USR_DEF_MAX_LOG_SIZE 4096
static char sv_strLog[USR_DEF_MAX_LOG_SIZE] = {0};
static char sv_strLogTemp[1024] = {0};
void YX_Dump_Hex( void * _pData, int _size, int _step)
{
    if ( _pData != NULL && _size> 0)
    {
        int i = 0;
        unsigned char * pData = (unsigned char * )_pData;
        memset( sv_strLog, 0, USR_DEF_MAX_LOG_SIZE);


        for ( i = 0; i < _size; ) {
            if ( i + _step > _size)
            {
                break;
            }
            sprintf( sv_strLogTemp, "%2d ~ %2d:", i, i + _step-1);
            strcat( sv_strLog, sv_strLogTemp);
            for (int j = 0; j < _step; ++j)
            {
                sprintf( sv_strLogTemp, "[%02x] ", pData[i + j]);
                strcat( sv_strLog, sv_strLogTemp);
            }
            strcat( sv_strLog, "\n");

            i += _step;
        }

        int left = _size - i;
        if ( left>0)
        {
            sprintf( sv_strLogTemp, "%2d ~ %2d:", i, _size-1);
            strcat( sv_strLog, sv_strLogTemp);
            for ( ; i < _size; ++i) {
                sprintf( sv_strLogTemp, "[%02x] ", pData[i]);
                strcat( sv_strLog, sv_strLogTemp);
            }
            strcat( sv_strLog, "\n");
        }
        YXLOGE( "YX_Dump_Hex:\n%s", sv_strLog);
    }
}




int YX_H264_Decode_extradata( void * _pInData, int _iInSize,
                              unsigned char * _pPPS, int * _iPPSSize,
                              unsigned char * _pSPS, int * _iSPSSIze)
{
    int spsstart = -1;
    int ppsstart = -1;
    int spslen;
    int ppslen;
    int i;
    int ret = -1;
    unsigned char * lv_pExtraData = (unsigned char*)_pInData;
    int lv_iExtraSize = _iInSize;
    char startCode[10] = { 0, 0, 0, 1};
    *_iPPSSize = 0;
    *_iSPSSIze = 0;
    if ( NULL==lv_pExtraData || 0 == lv_iExtraSize)
    {
        goto LABEL_FAIL;
    }

    for (i=0; i < lv_iExtraSize; i++)
    {
        if(lv_pExtraData[i] == 0x67 || lv_pExtraData[i]==0x27){
            if(i >= 2){
                spslen = lv_pExtraData[i-2]<<8 | lv_pExtraData[i-1];
                if(spslen > lv_iExtraSize){
                    continue;
                }
                spsstart = i;
                i = i+spslen;
            }
        }
        else if(lv_pExtraData[i] == 0x68 || lv_pExtraData[i] == 0x28)
        {
            if(i >= 2){
                ppslen = lv_pExtraData[i-2]<<8 | lv_pExtraData[i-1];
                if(ppslen > lv_iExtraSize){
                    continue;
                }
                ppsstart = i;
                i = i + ppslen;
            }
        }
    }

    if((spslen <= 0) ||  spsstart<0 ){
        YXLOGE("call %s %d failed for pps or sps", __FUNCTION__, __LINE__);
        goto LABEL_FAIL;
    }

    if((ppslen <=0) ||  ppsstart<0 ){
        YXLOGE("call %s %d failed for pps or sps", __FUNCTION__, __LINE__);
    }

    if((ppslen > 100) || (spslen >100)){
        YXLOGE("call %s %d failed for pps or sps", __FUNCTION__, __LINE__);
        goto LABEL_FAIL;
    }

    *_iPPSSize = ppslen + 4;
    memcpy( _pPPS, startCode, 4);
    memcpy(_pPPS+4, lv_pExtraData+ppsstart, ppslen);

    *_iSPSSIze = spslen + 4;
    memcpy( _pSPS, startCode, 4);
    memcpy(_pSPS+4, lv_pExtraData+spsstart, spslen);
    ret = 0;

    LABEL_FAIL:

    return ret;
}



uint32_t findStartCode(uint8_t* in_pBuffer, uint32_t in_ui32BufferSize,
                       uint32_t in_ui32Code, uint32_t& out_ui32ProcessedBytes)
{
    uint32_t ui32Code = in_ui32Code;

    const uint8_t * ptr = in_pBuffer;
    while (ptr < in_pBuffer + in_ui32BufferSize)
    {
        ui32Code = *ptr++ + (ui32Code << 8);
        if (is_start_code(ui32Code))
            break;
    }

    out_ui32ProcessedBytes = (uint32_t)(ptr - in_pBuffer);

    return ui32Code;
}


void parseH264SequenceHeader(uint8_t* in_pBuffer, uint32_t in_ui32Size,
                             uint8_t** inout_pBufferSPS, int& inout_ui32SizeSPS,
                             uint8_t** inout_pBufferPPS, int& inout_ui32SizePPS)
{
    uint32_t ui32StartCode = 0x0ff;

    uint8_t* pBuffer = in_pBuffer;
    uint32_t ui32BufferSize = in_ui32Size;

    uint32_t sps = 0;
    uint32_t pps = 0;

    do
    {
        uint32_t ui32ProcessedBytes = 0;
        ui32StartCode = findStartCode(pBuffer, ui32BufferSize, ui32StartCode, ui32ProcessedBytes);
        pBuffer += ui32ProcessedBytes;
        ui32BufferSize -= ui32ProcessedBytes;

        if (ui32BufferSize < 1)
            break;

        uint8_t val = (*pBuffer & 0x1f);

        if (val == 7)
            sps = ui32ProcessedBytes;

        if (val == 8)
            pps = sps+ui32ProcessedBytes;

    } while (ui32BufferSize > 0);

    *inout_pBufferSPS = in_pBuffer + sps - 4;
    inout_ui32SizeSPS = pps-sps;

    *inout_pBufferPPS = in_pBuffer + pps - 4;
    inout_ui32SizePPS = in_ui32Size - pps + 4;
}

int YX_H264_Decode_extradata_ex( void * _pInData, int _iInSize,
                                 unsigned char * _pPPS, int * _iPPSSize,
                                 unsigned char * _pSPS, int * _iSPSSIze
)
{
    uint8_t * pSPS = NULL;
    uint8_t * pPPS = NULL;
    int iPPSSize = 0, iSPSSize = 0;
    parseH264SequenceHeader( (uint8_t*)_pInData, _iInSize, &pSPS, iSPSSize, &pPPS, iPPSSize);

    if ( iSPSSize > 0)
    {
        memcpy( _pSPS, pSPS, iSPSSize);
    }

    if ( iPPSSize > 0)
    {
        memcpy( _pPPS, pPPS, iPPSSize);
    }
    *_iSPSSIze = iSPSSize;
    *_iPPSSize = iPPSSize;

    return 0;
}


void processExtraData2HVCC(uint8_t *_pExtraData, int *inout_iExtraSize, void * logCtx)
{
    //todo ...
#if 0
    int ret;
    struct SYXHEVCParamSets paramSets;
    struct SYXHEVCSEIContext seiContext;
    int isNaff = 0;
    int nalLen = 4;
    int err_recognition = 0;
    int apply_defdispwin = 1;
    memset( &paramSets, 0, sizeof(paramSets));
    memset( &seiContext, 0, sizeof(seiContext));
    ret = yx_hevc_decode_extradata( _pExtraData,
                                    *inout_iExtraSize,
                                    &paramSets,
                                    &seiContext,
                                    &isNaff, &nalLen,
                                    err_recognition,
                                    apply_defdispwin,
                                    logCtx);

    if ( ret >= 0)
    {
        ret = yx_hvcc_extradata_create( &paramSets, _pExtraData, inout_iExtraSize);
    }
    YXLOGE("yx_videotoolbox_hvcc_extradata_create return ret=%d", ret);
#endif
}
