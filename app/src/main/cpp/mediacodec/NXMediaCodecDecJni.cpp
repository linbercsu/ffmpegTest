//
//  on 2017/6/21.
//

#include "NXMediaCodecDecJni.h"
#include <jni.h>
#include <android/JniUtils/NXAndroidJniBase.h>

static jclass g_mediacodecDecClass = 0;
static JavaVM * g_pJavaVM = NULL;

#define USR_DEF_MAX_ITEM_COUNT 4

class NXAutoAttachJni
{
public:
    NXAutoAttachJni()
    :jniEnv(NULL)
    ,m_bNeedDetach(false)
    {
        Attach();
    }

    ~NXAutoAttachJni()
    {
        Detach();
        jniEnv = NULL;
    }

public:
    JNIEnv * getEnv()
    {
        return jniEnv;
    }

private:
    void Attach()
    {
        JavaVM* pJVM = g_pJavaVM;
        if(!pJVM)
            return ;

        int status = 0;
        status = pJVM->GetEnv((void **) (&jniEnv), JNI_VERSION_1_4);
        if (status < 0) {
            if (pJVM->AttachCurrentThread(&jniEnv, NULL) != JNI_OK) {
                return;
            }

            m_bNeedDetach = true;
        }

    }

    void Detach()
    {
        JavaVM* pJVM = g_pJavaVM;
        if(!pJVM)
            return ;
        if(m_bNeedDetach){
            if (pJVM->DetachCurrentThread() != JNI_OK) {
                YXLOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
            }
            m_bNeedDetach = false;
        }

    }

private:
    JNIEnv* jniEnv;
    bool m_bNeedDetach;
};

class NXMediaCodecDecJni {

public:
    NXMediaCodecDecJni();
    ~NXMediaCodecDecJni();

public:
    int initDecoder( int _width, int _height, unsigned char* sps, int spsSize, unsigned char* pps, int ppsSize);

    int setEncoder( int _width, int _height,  unsigned char* sps, int spsSize, unsigned char* pps, int ppsSize);

    int closeEncoder();

    int decodeFrame( unsigned char*  _frameData, int _inputSize, int64_t _timeStamp, int _outputTex);

    int64_t getInfoByFlag( int _inFlag);

    int flushDecoder();

private:
    struct YXByteArrayBuf
    {
    public:
        YXByteArrayBuf( )
        {
            m_jbyteArrayOutput = NULL;
            m_iMaxCapacity = 0;
        }

        ~YXByteArrayBuf()
        {
            ensureCapacity(0);
        }

    public:
        /**
         * 确保缓存大小;
         */
        void ensureCapacity( int _capacity)
        {
            NXAutoAttachJni autoAttach;
            JNIEnv * jniEnv = autoAttach.getEnv();

            if ( _capacity==0)
            {
                if ( NULL != m_jbyteArrayOutput)
                {

                    jniEnv->DeleteGlobalRef( m_jbyteArrayOutput);
                    m_jbyteArrayOutput = NULL;
                }
                m_iMaxCapacity = 0;
            }

            if ( m_iMaxCapacity < _capacity)
            {
                if ( NULL != m_jbyteArrayOutput)
                {
                    jniEnv->DeleteGlobalRef( m_jbyteArrayOutput);
                    m_jbyteArrayOutput = NULL;
                }

                jbyteArray tempOutputBuffer 	= jniEnv->NewByteArray( _capacity);
                m_jbyteArrayOutput = (jbyteArray)jniEnv->NewGlobalRef( tempOutputBuffer);
                jniEnv->DeleteLocalRef(tempOutputBuffer);
                m_iMaxCapacity = _capacity;
            }
        }

        void fillData(unsigned char * _pData, int _size)
        {
            if ( _size > 0)
            {
                NXAutoAttachJni autoAttach;
                JNIEnv * jniEnv = autoAttach.getEnv();

                ensureCapacity(_size);
                jniEnv->SetByteArrayRegion( m_jbyteArrayOutput, 0, _size, (jbyte*)_pData);
            }

        }

    public:
        /**
        * 输出数据用的缓存;
        */
        jbyteArray m_jbyteArrayOutput;
        /**
         * 缓存最大容量;
         */
        int m_iMaxCapacity;
    };



private:
    /**
     * 初始化对象;
     */
    void initContext();

    /**
     * 反初始化;
     */
    void unInitContext();

private:
    jobject m_jMediaCodecDec;
    YXByteArrayBuf * m_pBufPktData;
    YXByteArrayBuf * m_pBufSpsData;
    YXByteArrayBuf * m_pBufPpsData;

    jintArray m_jIntArray;

};

NXMediaCodecDecJni::NXMediaCodecDecJni()
        :m_jMediaCodecDec(NULL)
{

    m_jMediaCodecDec = NULL;
    m_jIntArray = NULL;

    initContext();

}

NXMediaCodecDecJni::~NXMediaCodecDecJni()
{
    unInitContext();
}


int NXMediaCodecDecJni::initDecoder(int _width, int _height, unsigned char* sps, int spsSize, unsigned char* pps, int ppsSize)
{
    NXAutoAttachJni autoAttach;
    JNIEnv * jniEnv = autoAttach.getEnv();

    jmethodID methodID = jniEnv->GetMethodID(g_mediacodecDecClass, "initDecoder", "(II[BI[BI)I");
    if ( spsSize>0)
    {
        m_pBufSpsData->fillData( sps, spsSize);
    }

    if ( ppsSize>0)
    {
        m_pBufPpsData->fillData( pps, ppsSize);
    }

    return jniEnv->CallIntMethod(m_jMediaCodecDec, methodID,
                                   _width,  _height,
                                   m_pBufSpsData->m_jbyteArrayOutput, spsSize,
                                   m_pBufPpsData->m_jbyteArrayOutput, ppsSize);

}

int NXMediaCodecDecJni::setEncoder(int _width, int _height, unsigned char* sps, int spsSize, unsigned char* pps, int ppsSize)
{
    NXAutoAttachJni autoAttach;
    JNIEnv * jniEnv = autoAttach.getEnv();

    jmethodID methodID = jniEnv->GetMethodID(g_mediacodecDecClass, "setEncoder", "(II[BI[BI)I");
    m_pBufSpsData->fillData( sps, spsSize);
    m_pBufPpsData->fillData( pps, ppsSize);

    return jniEnv->CallIntMethod(m_jMediaCodecDec, methodID,
                                   _width,  _height,
                                   m_pBufSpsData->m_jbyteArrayOutput, spsSize,
                                   m_pBufPpsData->m_jbyteArrayOutput, ppsSize);

}

int NXMediaCodecDecJni::closeEncoder()
{
    NXAutoAttachJni autoAttach;
    JNIEnv * jniEnv = autoAttach.getEnv();

    jmethodID methodID = jniEnv->GetMethodID(g_mediacodecDecClass, "closeEncoder", "()I");
    return jniEnv->CallIntMethod( m_jMediaCodecDec, methodID);
}

int NXMediaCodecDecJni::decodeFrame(unsigned char*  _frameData, int _inputSize, int64_t _timeStamp, int _outputTex)
{
    NXAutoAttachJni autoAttach;
    JNIEnv * jniEnv = autoAttach.getEnv();

    jmethodID methodID = jniEnv->GetMethodID(g_mediacodecDecClass, "decodeFrame", "([BIJI)I");
    if ( _inputSize > 0)
    {
        m_pBufPktData->fillData( _frameData, _inputSize);
    }

    return jniEnv->CallIntMethod( m_jMediaCodecDec, methodID, m_pBufPktData->m_jbyteArrayOutput,
                                    _inputSize, _timeStamp, _outputTex);

}

int64_t NXMediaCodecDecJni::getInfoByFlag(int _inFlag)
{
    NXAutoAttachJni autoAttach;
    JNIEnv * jniEnv = autoAttach.getEnv();

    jmethodID methodID = jniEnv->GetMethodID(g_mediacodecDecClass, "getInfoByFlag", "([II)I");
    int ret = jniEnv->CallIntMethod( m_jMediaCodecDec, methodID, m_jIntArray, _inFlag);
    int64_t lv_pts = 0;
    if ( ret == 2)
    {
        int tempBuf[2] = {0};
        jniEnv->GetIntArrayRegion( m_jIntArray, 0, 2, &tempBuf[0]);
        lv_pts = tempBuf[0];
        lv_pts |= ((int64_t)(tempBuf[1]&0xFFFFFFFF)<<32);
    }

    return lv_pts;
}

int NXMediaCodecDecJni::flushDecoder()
{
    NXAutoAttachJni autoAttach;
    JNIEnv * jniEnv = autoAttach.getEnv();

    jmethodID methodID = jniEnv->GetMethodID(g_mediacodecDecClass, "flushDecoder", "()I");
    int ret = jniEnv->CallIntMethod( m_jMediaCodecDec, methodID);
    return ret;
}


void NXMediaCodecDecJni::initContext()
{
    if ( NULL == g_mediacodecDecClass)
    {
        YXLOGE( "NXAvcDecoder 类未初始化！！！");
        return;
    }

    NXAutoAttachJni autoAttach;
    JNIEnv * jniEnv = autoAttach.getEnv();

    jmethodID fontTextConstructor = jniEnv->GetMethodID(g_mediacodecDecClass, "<init>", "()V");
    jobject objCtx = jniEnv->NewObject(g_mediacodecDecClass, fontTextConstructor);
    m_jMediaCodecDec = jniEnv->NewGlobalRef(objCtx);
    jniEnv->DeleteLocalRef(objCtx);

    // 输出缓存配置;
    m_pBufPktData = new YXByteArrayBuf();
    m_pBufPktData->ensureCapacity( 1024*1024*4);

    m_pBufPpsData = new YXByteArrayBuf();
    m_pBufPpsData->ensureCapacity( 256);

    m_pBufSpsData = new YXByteArrayBuf();
    m_pBufSpsData->ensureCapacity( 256);

    jintArray lv_jIntArray = jniEnv->NewIntArray( 10);
    m_jIntArray = (jintArray)jniEnv->NewGlobalRef(lv_jIntArray);
    jniEnv->DeleteLocalRef(lv_jIntArray);

}


void NXMediaCodecDecJni::unInitContext()
{
    NXAutoAttachJni autoAttach;
    JNIEnv * jniEnv = autoAttach.getEnv();

    jniEnv->DeleteGlobalRef(m_jIntArray);
    m_jIntArray = NULL;

    if ( NULL != m_pBufPktData)
    {
        delete m_pBufPktData;
        m_pBufPktData = NULL;
    }

    if ( NULL != m_pBufSpsData)
    {
        delete  m_pBufSpsData;
        m_pBufSpsData = NULL;
    }

    if ( NULL != m_pBufPpsData)
    {
        delete m_pBufPpsData;
        m_pBufPpsData = NULL;
    }

    if ( NULL != m_jMediaCodecDec)
    {
        jniEnv->DeleteGlobalRef( m_jMediaCodecDec);
        m_jMediaCodecDec = NULL;
    }

}

bool YXMediaCodecDecJniLoadClass( void * _env)
{
    /*
    JNIEnv * jniEnv = (JNIEnv*)_env;
    if(g_mediacodecDecClass){
        jniEnv->DeleteGlobalRef(g_mediacodecDecClass);
        g_mediacodecDecClass = 0;
    }

    jclass clsAvcDecText = jniEnv->FindClass("com/nxinc/videoedit/VMediacodec/NXAvcDecoder");
    if(jniEnv->ExceptionCheck())
        jniEnv->ExceptionClear();

    if(!clsAvcDecText){
        YXLOGE("get NXAvcDecoder class failed!");
        return false;
    }

    g_mediacodecDecClass = static_cast<jclass>(jniEnv->NewGlobalRef(clsAvcDecText));
    jniEnv->DeleteLocalRef(clsAvcDecText);

    return true;
     */
    return true;
}

bool YXMediaCodecDecJniLoadClass( void * _pGloabJvm, int _jniVersion)
{
    /*
    JNIEnv* jniEnv = NULL;
    g_pJavaVM = (JavaVM*)_pGloabJvm;
    JavaVM * vm = g_pJavaVM;
    //判断一下JNI的版本
    if (vm->GetEnv((void**) &jniEnv, _jniVersion) != JNI_OK)
    {
        // LOGE("ERROR: GetEnv failed\n");
        return false;
    }

    if(g_mediacodecDecClass){
        jniEnv->DeleteGlobalRef(g_mediacodecDecClass);
        g_mediacodecDecClass = 0;
    }

    jclass clsAvcDecText = jniEnv->FindClass("com/nxinc/videoedit/VMediacodec/NXAvcDecoder");
    if(jniEnv->ExceptionCheck())
        jniEnv->ExceptionClear();

    if(!clsAvcDecText){
        YXLOGE("get NXAvcDecoder class failed!");
        return false;
    }

    g_mediacodecDecClass = static_cast<jclass>(jniEnv->NewGlobalRef(clsAvcDecText));
    jniEnv->DeleteLocalRef(clsAvcDecText);

    return true;
     */
    return true;
}



SYXMediaCodecDecContext * YX_MediaCodecDec_Alloc()
{
    SYXMediaCodecDecContext * pCtx = new SYXMediaCodecDecContext;
    pCtx->mediacodecCtx = new NXMediaCodecDecJni;
    return pCtx;
}

void YX_MediaCodecDec_Delete( SYXMediaCodecDecContext ** _ppCtx)
{
    if ( NULL != _ppCtx)
    {
        SYXMediaCodecDecContext * pCtx = *_ppCtx;
        if ( NULL != pCtx)
        {
            NXMediaCodecDecJni * pDecCtx = (NXMediaCodecDecJni*)pCtx->mediacodecCtx;
            if ( NULL != pDecCtx)
            {
                delete pDecCtx;
            }
            delete pCtx;
        }
        *_ppCtx = NULL;
    }
}

int YX_MeidaCodecDec_init( SYXMediaCodecDecContext ** _ppCtx,
                           int _width, int _height,
                           unsigned char* sps, int spsSize,
                           unsigned char* pps, int ppsSize)
{
    int ret;
    SYXMediaCodecDecContext * pCtx = YX_MediaCodecDec_Alloc();
    NXMediaCodecDecJni * pDecCtx = (NXMediaCodecDecJni*)pCtx->mediacodecCtx;
    ret = pDecCtx->initDecoder( _width, _height, sps, spsSize, pps, ppsSize);
    if ( !ret )
    {
        *_ppCtx = pCtx;

    } else
    {
        YX_MediaCodecDec_Delete( &pCtx);
        *_ppCtx = NULL;
    }
    return ret;
}

int YX_MediaCodecDec_set( SYXMediaCodecDecContext * _ctx,
                          int _width, int _height,
                          unsigned char* sps, int spsSize,
                          unsigned char* pps, int ppsSize)
{
    NXMediaCodecDecJni * pDecCtx = (NXMediaCodecDecJni*)_ctx->mediacodecCtx;
    return pDecCtx->setEncoder( _width, _height, sps, spsSize, pps, ppsSize);
}

int YX_MediaCodecDec_close( SYXMediaCodecDecContext ** _ppCtx)
{
    int ret = 0;
    if ( _ppCtx != NULL)
    {
        SYXMediaCodecDecContext * pCtx = *_ppCtx;
        NXMediaCodecDecJni * pDecCtx = (NXMediaCodecDecJni*)pCtx->mediacodecCtx;
        ret = pDecCtx->closeEncoder();
        YX_MediaCodecDec_Delete(_ppCtx);
    }
    YXLOGE( "call %s %d", __FUNCTION__, __LINE__);
    return ret;
}

int YX_MediaCodecDec_decode_frame( SYXMediaCodecDecContext * _ctx,
                                   unsigned char*  _frameData, int _inputSize,
                                   long long _timeStamp, int _outputTex)
{
    NXMediaCodecDecJni * pDecCtx = (NXMediaCodecDecJni*)_ctx->mediacodecCtx;
    return pDecCtx->decodeFrame( _frameData, _inputSize, _timeStamp, _outputTex);
}

long long YX_MediaCodec_get_info_by_flag( SYXMediaCodecDecContext * _ctx, int _flag)
{
    NXMediaCodecDecJni * pDecCtx = (NXMediaCodecDecJni*)_ctx->mediacodecCtx;
    return pDecCtx->getInfoByFlag( _flag);

}


int YX_MediaCodecDec_flush(SYXMediaCodecDecContext * _ctx)
{
    NXMediaCodecDecJni * pDecCtx = (NXMediaCodecDecJni*)_ctx->mediacodecCtx;

    return pDecCtx->flushDecoder();

}