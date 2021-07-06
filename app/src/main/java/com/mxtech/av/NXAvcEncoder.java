package com.mxtech.av;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;

import java.nio.ByteBuffer;
import java.util.Locale;
import java.util.concurrent.ArrayBlockingQueue;

import static android.media.MediaCodecInfo.CodecProfileLevel.AVCLevel3;
import static android.media.MediaCodecInfo.CodecProfileLevel.AVCLevel31;
import static android.media.MediaCodecInfo.CodecProfileLevel.AVCProfileBaseline;
import static android.media.MediaCodecInfo.CodecProfileLevel.AVCProfileConstrainedBaseline;

/**
 * 16/8/16.
 */
@SuppressLint("NewApi")
public class NXAvcEncoder {
    private static final String TAG 				= 	"AvcMediaEncoder";
    private String VIDEO_MIME_TYPE 	                = 	MediaFormat.MIMETYPE_VIDEO_AVC;
    private static final int 	TIMEOUT_USEC 		= 	0;
    private static final int	maskBitRate			=	0x1;
    private static final int	maskFrameRate		=	0x1<<1;
    private static final int	maskForceRestart	=	0x1<<2;
    private static final int    minFrameRate        =   7;
    private static final int    maxFrameRate        =   2000;


    /*
     * Log switch
     */
    private static boolean m_verbose     = true;
    private static boolean m_testRuntime = false;
    private static boolean m_bSaveAvc    = false;

    /*
     * The start time of encoding
     */
    private long        m_startTime    = 0;
    private  boolean    m_bSuccessInit = false;


    /*
     * MediaCodec related info
     */
    private MediaCodec m_mediaCodec 	 = null;
    private MediaFormat m_codecFormat	 = null;
    private MediaCodecInfo m_codecInfo 	 = null;
    private Boolean m_useInputSurface    = false;
    private long	m_getnerateIndex     = 0;
    private boolean m_bSignalEndOfStream = false;
    private boolean m_bNeedSingalEnd     = false;

    /*
     * Encoder thread related info
     */
    public Thread encoderThread	= null;
    public Boolean isRunning	= false;

    /*
     * Asynchronous encoding texture input queue
     */
    private static final int inputpacketsize = 10;
    private static ArrayBlockingQueue<int[]> inputQueue = new ArrayBlockingQueue<>(inputpacketsize);

    /*
     * Asynchronous encoding memory input queue
     */
    private static ArrayBlockingQueue<byte[]> inputQueue2 = new ArrayBlockingQueue<>(inputpacketsize);

    /*
     * Encoder output queue
     */
	private static int avcqueuesize = 25;
	public static ArrayBlockingQueue<CodecData> AVCQueue = new ArrayBlockingQueue<CodecData>(avcqueuesize);
	private CodecData  mLastCodecData		= null;


    public byte[]	configbyte	     = null;
    public Boolean isNeedReconfigure = false;
    public int		configStatus	 = 0;
    private byte[]  sps;
    private byte[]  pps;


    /*
     * Encoder parameters
     */
    public int 	width				   = 640;
    public int	height				   = 480;
    public int  frameRate			   = 25;
    public int	bitRate				   = 2500000;
    public int  mKeyFrameInternal      = 1;
    public int  colorFormat			   = MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar;
    public int  profile                = 0;
    public boolean skipGenKey          = true;
    public boolean isHuaweiP9AndHonor8 = false;

    private static NXAvcEncoder curObj = null;

    public static NXAvcEncoder createEncoderObject()
    {
        curObj = new NXAvcEncoder();
        return curObj;
    }

    /*
     * API for FFmpeg encoder
     */
    public int initEncoder( int _width, int _height, int _frameRate, int _colorFormat, int _iFrameInternal, int _bitRate, int _profile, boolean _bUseInputSurface, int _encType)
    {
        m_bSuccessInit = false;
        if ( m_useInputSurface && Build.VERSION.SDK_INT<18 )
        {
            return -1;
        }

        int err = 0;

        if ( _encType == 0)
        {
            VIDEO_MIME_TYPE = MediaFormat.MIMETYPE_VIDEO_AVC;
        }
        else if ( _encType == 1)
        {
            VIDEO_MIME_TYPE = MediaFormat.MIMETYPE_VIDEO_HEVC;
        }

        try
        {
            enumCodecList();
            MediaCodec codec = MediaCodec.createEncoderByType( VIDEO_MIME_TYPE );
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return -1;
        }

        configbyte = null;
        m_bSignalEndOfStream = false;
        m_bNeedSingalEnd = false;
        String info = String.format( Locale.getDefault(),
                "wxh:[%d]X[%d] _frameRate=%d _iFrameInternal=%d _bitRate=%d",
                _width, _height, _frameRate, _iFrameInternal, _bitRate);
        Log.e( TAG, "initEncoder "+info);
        if ( skipGenKey )
        {
            m_useInputSurface = _bUseInputSurface;
            if ( m_useInputSurface)
            {
                _colorFormat = MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface;
            }
            setEncoder(_width, _height, _frameRate, _bitRate, _iFrameInternal, _colorFormat, _profile);
            isNeedReconfigure = true;
            m_bSuccessInit = true;
            Log.e(TAG, "Java call initEncoder finished [skip generate key info]!!! err:" + err);
        }
        else
        {
            // 先用模拟数据生成关键数据;
            int lv_iColorFormat = getSupportedColorFormat();
            m_useInputSurface = false;

            setEncoder(_width, _height, _frameRate, _bitRate, _iFrameInternal, lv_iColorFormat, _profile);
            startEncoder();
            err = generateExtraData();
            stopEncoder();
            if ( err >=0 )
            {
                // 生成关键数据需要模拟内存数据，所以在模拟内存数据生成完成后置为目标状态；
                m_useInputSurface = _bUseInputSurface;
                if(m_useInputSurface)
                {
                    _colorFormat = MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface;
                }
                setEncoder(_width, _height, _frameRate, _bitRate, _iFrameInternal, _colorFormat, _profile);


                if(m_useInputSurface)
                {
                    err = exceptionCheck();
                }
                else
                {
                    err = 0;
                }

                m_bSuccessInit = err >= 0;

            }
            Log.e(TAG, "Java call initEncoder finished!!! err:" + err);
        }


        return err;
    }


    /*
     * Set encoding parameters.
     */
    public int setEncoder( int _width, int _height, int _frameRate, int _bitRate, int _iFrameInternal, int _iColorFormat, int _profile)
    {
        configStatus = 0;
        if ( _width > 0 )
        {
            width = _width;
        }

        if ( _height > 0 )
        {
            height = _height;
        }


        if ( _frameRate > 0)
        {
            if ( _frameRate < minFrameRate )
            {
                String str = String.format( Locale.getDefault(), "_frameRate:[%d] is too small, change to %d", _frameRate, minFrameRate );
                Log.e( TAG, str );
                _frameRate = minFrameRate;
            }
            else if ( _frameRate > maxFrameRate)
            {
                String str = String.format( Locale.getDefault(), "_frameRate:[%d] is too large, change to %d", _frameRate, maxFrameRate );
                Log.e( TAG, str );
                _frameRate = maxFrameRate;
            }

            if ( frameRate != _frameRate )
            {
                frameRate	= _frameRate;
                if ( frameRate < mKeyFrameInternal)
                {
                    mKeyFrameInternal = frameRate;
                }
                isNeedReconfigure = true;
                configStatus |= maskFrameRate;
            }
        }

        if ( _bitRate>0 && bitRate != _bitRate )
        {
            bitRate		=	_bitRate;
            isNeedReconfigure = true;
            configStatus |= maskBitRate;
        }

        if ( _iFrameInternal >= 0 )
            mKeyFrameInternal = _iFrameInternal;

        if ( _iColorFormat > 0 )
        {
            colorFormat	= _iColorFormat;
        }

        if ( _profile >= 0 )
        {
            profile = _profile;
        }

        return 0;
    }


    /*
     * Get extra data
     */
    public int getExtraData( byte[] output)
    {
        int length = 0;
        if ( null != output && null != configbyte && output.length >= configbyte.length )
        {
            System.arraycopy( configbyte, 0, output, 0, configbyte.length );
            length = configbyte.length;
        }

        YXLog( String.format( "output%c=null configbyte%c=null", output == null? '=' : '!', configbyte == null ? '=' : '!' ) );

        return length;
    }

    /*
     * Log output
     */
    public static void YXLog(String info)
    {
        Log.i(TAG, info);
    }

    /*
     * Generate extra data.
     */
    private int generateExtraData()
    {
        int lv_iYSize = width * height;
        int lv_iYUVSize = lv_iYSize * 3 / 2;
        byte[] yuvData = new byte[lv_iYUVSize];
        byte[] avcData = new byte[lv_iYUVSize];
        int lv_iCount = 0;
        int err = 0;
        while( configbyte==null)
        {
            err = encodeVideoFromBuffer(yuvData, avcData, 0);
            if ( err<0)
            {
                break;
            }

             if ( configbyte==null)
            {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
            ++lv_iCount;
            if( lv_iCount>=10)
            {
            	break;
            }

        }
        YXLog(String.format("generateExtraData %s !!!", configbyte==null?"failed":"succeed"));

        isNeedReconfigure = true;
        configStatus |= maskForceRestart;
        return err;
    }

    private int exceptionCheck()
    {
        int err = configbyte==null ? -1 : 0;
        try {
            reconfigureMediaFormat();
            m_mediaCodec = MediaCodec.createEncoderByType(VIDEO_MIME_TYPE);
            m_mediaCodec.configure(m_codecFormat, null, null,
                    MediaCodec.CONFIGURE_FLAG_ENCODE);
            m_mediaCodec.start();
            m_mediaCodec.stop();
            m_mediaCodec.release();
            m_mediaCodec = null;
            YXLog(String.format("exceptionCheck succeed !!!"));

        } catch (Exception e) {
            e.printStackTrace();
            err = -1;
            YXLog(String.format("exceptionCheck failed !!!"));
        }
        return err;

    }

    // 根据编码参数生成编码格式信息；
    private int reconfigureMediaFormat()
    {
        if( m_verbose)
        {
            Log.d(TAG, "call reconfigureMediaFormat !!!");
        }

        MediaCodecInfo.CodecCapabilities capabilities = m_codecInfo
                .getCapabilitiesForType(VIDEO_MIME_TYPE);
//
        MediaCodecInfo.CodecProfileLevel[] levels = capabilities.profileLevels;

        m_codecFormat = MediaFormat.createVideoFormat( VIDEO_MIME_TYPE, width, height);
        m_codecFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, colorFormat);
        m_codecFormat.setInteger(MediaFormat.KEY_BIT_RATE, bitRate);
        m_codecFormat.setInteger(MediaFormat.KEY_FRAME_RATE, frameRate);
        m_codecFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, mKeyFrameInternal);
        m_codecFormat.setInteger(MediaFormat.KEY_BITRATE_MODE, MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_VBR);
//        CodecInfo infoEnc = CodecInfo.getSupportedFormatInfo(encoderName, mimeType, w, h, MAX_FPS);

        boolean profileSet = false;
        for (MediaCodecInfo.CodecProfileLevel level : levels) {
            if (level.profile == AVCProfileBaseline) {
                if (level.level == AVCLevel3 || level.level == AVCLevel31) {
                    m_codecFormat.setInteger(MediaFormat.KEY_PROFILE, level.profile);
                    m_codecFormat.setInteger(MediaFormat.KEY_LEVEL, level.level);
                    profileSet = true;
                    break;
                }

            }
        }

        if (!profileSet) {
            for (MediaCodecInfo.CodecProfileLevel level : levels) {
                if (level.profile == AVCProfileBaseline) {
                    if (level.level >= AVCLevel3) {
                        m_codecFormat.setInteger(MediaFormat.KEY_PROFILE, level.profile);
                        m_codecFormat.setInteger(MediaFormat.KEY_LEVEL, level.level);
                        profileSet = true;
                        break;
                    }

                }
            }
        }

        if (!profileSet) {
            for (MediaCodecInfo.CodecProfileLevel level : levels) {
                if (level.profile == AVCProfileConstrainedBaseline) {
                    m_codecFormat.setInteger(MediaFormat.KEY_PROFILE, level.profile);
                    m_codecFormat.setInteger(MediaFormat.KEY_LEVEL, level.level);
                    profileSet = true;
                    break;

                }
            }
        }
//        m_codecFormat.setInteger(MediaFormat.KEY_PROFILE, levels[4].profile);
//        m_codecFormat.setInteger(MediaFormat.KEY_LEVEL, levels[4].level);

        Log.i(TAG, String.format( "width:[%d] height:[%d] frameRate:[%d] iFrameInternal:[%d] bitRate:[%d] colorFormat:[%d]", width, height, frameRate, mKeyFrameInternal, bitRate, colorFormat));

        return 0;
    }

    // 放入图像数据并取出编码后的数据；
    @SuppressLint( "NewApi")
    public int encodeVideoFromBuffer( byte[] input, byte[] output, long pts)
    {
        // 重新配置编码器;
        if (isNeedReconfigure)
        {
            if( configStatus==maskBitRate && Build.VERSION.SDK_INT>=19)// SDK_INT >= 19 支持动态码率设置，不需要重新配置编码器
            {
                Bundle config = new Bundle();
                config.putInt( MediaCodec.PARAMETER_KEY_VIDEO_BITRATE, bitRate);
				m_mediaCodec.setParameters(config);
                configStatus = 0;
            }
            else
            {
                restartEncoder();
            }

            isNeedReconfigure = false;
        }
        
        drainOutputBuffer();

        int inputBufferIndex;

        try
        {
            inputBufferIndex = m_mediaCodec.dequeueInputBuffer(-1);
            if (inputBufferIndex >= 0) {
//                long pts = computePresentationTime(m_getnerateIndex);
                ByteBuffer inputBuffer = getInputBufferByIdx(inputBufferIndex);
                inputBuffer.clear();
                inputBuffer.put(input);
                m_mediaCodec.queueInputBuffer(inputBufferIndex, 0, input.length, pts, 0);
                ++m_getnerateIndex;
            }

            drainOutputBuffer();
            mLastCodecData = AVCQueue.poll();
            int length = 0;
            if( null != output && null != mLastCodecData)
            {
                length = mLastCodecData.data.length;
                System.arraycopy(mLastCodecData.data, 0, output, 0, length);
            }

            return length;

        }
        catch (Exception e)
        {
            e.printStackTrace();
            return -1;
        }
    }

    public int encodeVideoFromBufferAsyn( byte[] input, byte[] output)
    {
        if( null==encoderThread)
        {
            startEncoderThread();
        }

        if( null != input)
        {
            if( inputQueue2.size()>=inputpacketsize)
            {
                inputQueue2.poll();
            }
            inputQueue2.add(input);
        }

        byte[] data = inputQueue2.poll();
        int length = 0;
        if( null != output && null != data)
        {
            System.arraycopy(data, 0, output, 0, data.length);
            length = data.length;
        }

        return length;

    }

    public int  getLastFrameFlags()
    {
        if( null != mLastCodecData)
        {
            return mLastCodecData.flag;
        }
        return 0;
    }
    
    private void addOutputData( byte[] _data, long _pts, int _flag)
    {
//        CodecData data = new CodecData();
//        data.data 	= _data;
//        data.pts	= _pts;
//        data.flag   = _flag;


        int l = _data.length;
        byte[] tmpdata;
        CodecData data = new CodecData();

        if (isHuaweiP9AndHonor8 && (_data[l - 1] == 0) && (_data[l - 2] == 0) && (_data[l - 3] == 0) && (_data[l - 4] == 0)) {
            tmpdata = new byte[_data.length - 8];
            System.arraycopy(_data, 0, tmpdata, 0, _data.length - 8);
            data.data 	= tmpdata;
        }else{
            data.data 	= _data;
        }

        data.pts	= _pts; //input: nano output:milli
        data.flag   = _flag;

        try
        {
            AVCQueue.add(data);
        }
        catch ( Exception e)
        {
            e.printStackTrace();
        }
    }

    @SuppressLint("NewApi")
    private void drainOutputBuffer()
    {
        MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();

        int outputBufferIndex = MediaCodec.INFO_TRY_AGAIN_LATER;
        try
        {
            outputBufferIndex = m_mediaCodec.dequeueOutputBuffer( bufferInfo, TIMEOUT_USEC);
        }
        catch ( Exception e)
        {
            e.printStackTrace();
        }

        while (outputBufferIndex >= 0)
        {
            // Log.i("AvcEncoder",
            // "Get H264 Buffer Success! flag = "+bufferInfo.flags+",pts = "+bufferInfo.presentationTimeUs+"");
            ByteBuffer outputBuffer = getOutputBufferByIdx(outputBufferIndex);
            byte[] outData = new byte[bufferInfo.size];
            outputBuffer.position(bufferInfo.offset);
            outputBuffer.limit(bufferInfo.offset
                    + bufferInfo.size);
            outputBuffer.get(outData);

            if (bufferInfo.flags == MediaCodec.BUFFER_FLAG_CODEC_CONFIG)
            {
                configbyte = outData;
            }
            else if (bufferInfo.flags == MediaCodec.BUFFER_FLAG_KEY_FRAME)
            {
                if ( configbyte!=null)
                {
                    // I帧数据里面包含关键头数据，为了统一输出格式，在这里将其去掉；
                    if( outData[4] == configbyte[4] && (outData[configbyte.length+4]&0x1f)==5)
                    {
                        byte[] clipData = new byte[outData.length-configbyte.length];
                        System.arraycopy( outData, configbyte.length, clipData, 0, clipData.length);
                        outData = clipData;
                    }
                }
                else
                {
                    // TODO:可能某种些手机通过两种方式都未获取到关键数据，那么这个时候关键数据一定存放在I帧里面
                    // TODO:这个时候需要我们直接从I帧里面提取；
                    Log.e( TAG, "I can't find configbyte!!!! NEED extract from I frame!!!");
                }

                addOutputData( outData, bufferInfo.presentationTimeUs, bufferInfo.flags);

            }
            else if( bufferInfo.flags == MediaCodec.BUFFER_FLAG_END_OF_STREAM)
            {
                break;
            }
            else
            {
            	addOutputData( outData, bufferInfo.presentationTimeUs, bufferInfo.flags);
            }

            m_mediaCodec.releaseOutputBuffer(outputBufferIndex,	false);
            outputBufferIndex = m_mediaCodec.dequeueOutputBuffer(bufferInfo, TIMEOUT_USEC);
        }

        if( outputBufferIndex== MediaCodec.INFO_OUTPUT_FORMAT_CHANGED)
        {
            MediaFormat format =  m_mediaCodec.getOutputFormat();
            ByteBuffer csd0 = format.getByteBuffer("csd-0");
            ByteBuffer csd1 = format.getByteBuffer("csd-1");
            if ( csd0 != null && csd1 != null)
            {
                int configLength = 0;
                sps = csd0.array().clone();
                pps = csd1.array().clone();
                configLength = sps.length + pps.length;
                configbyte = new byte[configLength];
                System.arraycopy( sps, 0, configbyte, 0, sps.length);
                System.arraycopy( pps, 0, configbyte, sps.length, pps.length);
            }
            else if( csd0 != null)
            {
                configbyte = csd0.array().clone();
            }
        }
    }

    private void dumpHex(String tag, byte[] _data)
    {
        dumpHex( tag, _data, 0, _data.length);
    }

    private void dumpHex(String tag, byte[] _data, int _start, int _end)
    {
        int i;
        int step = 4;
        _start = _start > 0 ? _start : 0;
        _end = _end <= _data.length ? _end : _data.length;
        String outPut = "dumpHex:";
        outPut += String.format(Locale.CHINA, "[%s][%d]\n", tag, _data.length);
        for ( i=_start; i<_end; )
        {
            if ( _end -i >= step)
            {
                int j;
                outPut += String.format(Locale.CHINA, "%2d ~ %2d ", i, i+step-1);
                for ( j=0; j<step; ++j)
                {
                    outPut += String.format( "[%02x] ", _data[i+j]);
                }
                outPut += "\n";
                i += step;
            }
            else
            {
                break;
            }

        }
        int left = _end - i;
        int j;
        if ( left > 0)
        {
            outPut += String.format(Locale.CHINA, "%2d ~ %2d ", i, i+left-1);
            for ( j=0; j<left; ++j)
            {
                outPut += String.format( "[%02x] ", _data[i+j]);
            }
            outPut += "\n";
        }
        Log.e( TAG, outPut);
    }

    // 放入图像纹理并取得编码后的数据;
    /*
     *
     */
    @SuppressLint("NewApi")
    public int encodeVideoFromTexture( int[] input, byte[] output)
    {
        return 0;
    }



    @SuppressLint("NewApi")
    public int encodeVideoFromTextureAsyn( int[] input, byte[] output)
    {
        return 0;
    }

    public Surface getInputSurface()
    {
        return null;
    }


    /*
     * Start encoder.
     */
    public int startEncoder()
    {
        String model = Build.MODEL;
        String manufacturer = Build.MANUFACTURER;
        String radioVersion = Build.getRadioVersion();

        if ( (manufacturer.trim().contains("HUAWEI") && model.trim().contains("EVA-AL00"))
                || (manufacturer.trim().contains("HUAWEI") && model.trim().contains("FRD-AL00"))
                ){
            isHuaweiP9AndHonor8 = true;
        }else {
            isHuaweiP9AndHonor8 = false;

        }

        if( m_verbose)
        {
            Log.d(TAG, "call startEncoder !!!");
        }

        try {
            reconfigureMediaFormat();
            m_mediaCodec = MediaCodec.createEncoderByType(VIDEO_MIME_TYPE);
            m_mediaCodec.configure(m_codecFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);

            m_mediaCodec.start();
//            m_startTime = System.nanoTime();
            m_startTime = 0;
            isNeedReconfigure = false;

        } catch (Exception e) {
            e.printStackTrace();
            return -1;
        }

        return 0;
    }

    /*
     * Stop encoder.
     */
    public void stopEncoder()
    {
        if( m_verbose)
        {
            Log.d(TAG, "call stopEncoder !!!");
        }
		try {

            AVCQueue.clear();

            if (null != m_mediaCodec) {
				m_mediaCodec.stop();
				m_mediaCodec.release();
				m_mediaCodec = null;
			}


		} catch (Exception e) {
			e.printStackTrace();
		}

    }

    /*
     * Restart encoder.
     */
    public int restartEncoder()
    {
        if( m_verbose)
        {
            Log.d(TAG, "call restartEncoder !!!");
        }
        m_bNeedSingalEnd = false;
        stopEncoder();
        startEncoder();
        return 0;
    }
    
	public int closeEncoder()
	{
        stopEncoder();

 		Log.e(TAG, "Java call closeEncoder finished!!!");

		return 0;
	}


    public int closeEncoderAsyn()
    {
        stopEncoderThread();
        encoderThread = null;
        return 0;
    }
	
	// 用于获取信息;
	public int getInfoByFlag( int[] _outBuf, int _inFlag)
	{
		int status = -1;
		// 假装用1代表获取时间戳;
		if( _inFlag==1)
		{
			_outBuf[0] = (int)(mLastCodecData.pts&0xFFFFFFFF);
			_outBuf[1] = (int)((mLastCodecData.pts>>32)&0xFFFFFFFF);

			status = 2;
		}
		
		return status;
	}

	public long getLastPts() {
        return mLastCodecData.pts;
    }


    @SuppressWarnings("deprecation")
    private ByteBuffer getInputBufferByIdx( int index )
    {
        if ( Build.VERSION.SDK_INT >= 21 )
        {
            return m_mediaCodec.getInputBuffer( index );
        }
        else
        {
            ByteBuffer[] inputBuffers = m_mediaCodec.getInputBuffers();
            return inputBuffers[index];
        }
    }

    @SuppressWarnings("deprecation")
    private ByteBuffer getOutputBufferByIdx( int index )
    {
        if ( Build.VERSION.SDK_INT >= 21 )
        {
            return m_mediaCodec.getOutputBuffer( index );
        }
        else
        {
            ByteBuffer[] outputBuffers = m_mediaCodec.getOutputBuffers();
            return outputBuffers[index];
        }
    }

    /**
     * Generates the presentation time for frame N, in microseconds.
     */
    private long computePresentationTime( long frameIndex )
    {

        return m_startTime + ( frameIndex * 1000000 / frameRate );
    }


    @SuppressWarnings("deprecation")
    private static MediaCodecInfo selectCodec( String mimeType )
    {
        int numCodecs = MediaCodecList.getCodecCount();
        for ( int i = 0; i < numCodecs; i++ )
        {
            /*
             * Skip over non-encoders.
             */
            MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt( i );
            if ( !codecInfo.isEncoder() )
            {
                continue;
            }

            String[] types = codecInfo.getSupportedTypes();
            for ( String type : types )
            {
                if ( type.equalsIgnoreCase( mimeType ) )
                {
                    Log.i( TAG, "codecInfo[" + i + "].name=" + codecInfo.getName() );
                    return codecInfo;
                }
            }
        }
        return null;
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    private static MediaCodecInfo findCodec( String mimeType )
    {
        MediaCodecList mediaLst = new MediaCodecList(MediaCodecList.REGULAR_CODECS);
        MediaCodecInfo[] codecInfos = mediaLst.getCodecInfos();
        for( MediaCodecInfo codecInfo : codecInfos )
        {
            /*
             * Skip over non-encoders.
             */
            if ( !codecInfo.isEncoder() )
            {
                continue;
            }

            String[] types = codecInfo.getSupportedTypes();
            for ( String type : types )
            {
                if ( type.equalsIgnoreCase( mimeType ) )
                {
                    Log.i( TAG, "codecInfo.name=" + codecInfo.getName() );
                    return codecInfo;
                }
            }
        }
        return null;
    }

    private static void enumCodecList()
    {
        MediaCodecList mediaLst = new MediaCodecList( MediaCodecList.REGULAR_CODECS );
        MediaCodecInfo[] codecInfos = mediaLst.getCodecInfos();
        for( MediaCodecInfo codecInfo : codecInfos )
        {
            /*
             * Skip over non-encoders.
             */
            if ( !codecInfo.isEncoder() )
            {
                continue;
            }

            Log.i( TAG, "Encoder name " + codecInfo.getName() );
            String[] types = codecInfo.getSupportedTypes();
            for ( String type : types )
            {
                Log.i( TAG, "\tsupported media type " + type );
            }
        }
    }

    // support these color space currently
    private static boolean isRecognizedFormat(int colorFormat) {
        switch (colorFormat) {
            case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar:
            case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar:
                return true;
            default:
                return false;
        }
    }

    // if returned -1, can't find a suitable color format
    public int getSupportedColorFormat() {
        String manufacturer = Build.MANUFACTURER;
        String model = Build.MODEL;

        if (m_verbose)
            Log.d(TAG, "manufacturer = " + manufacturer + " model = " + model);

        if( Build.VERSION.SDK_INT>=21)
        {
            m_codecInfo = findCodec(VIDEO_MIME_TYPE);
        }
        else
        {
            m_codecInfo = selectCodec(VIDEO_MIME_TYPE);
        }

        if ((manufacturer.compareTo("Xiaomi") == 0) // for Xiaomi MI 2SC,
                // selectCodec methord is
                // too slow
                && (model.compareTo("MI 2SC") == 0))
            return MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar;

        if ((manufacturer.compareTo("Xiaomi") == 0) // for Xiaomi MI 2,
                // selectCodec methord is
                // too slow
                && (model.compareTo("MI 2") == 0))
            return MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar;

        if ((manufacturer.compareTo("samsung") == 0) // for samsung S4,
                // COLOR_FormatYUV420Planar
                // will write green
                // frames
                && (model.compareTo("GT-I9500") == 0))
            return MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar;

        if ((manufacturer.compareTo("samsung") == 0) // for samsung 混手机,
                // COLOR_FormatYUV420Planar
                // will write green
                // frames
                && (model.compareTo("GT-I9300") == 0))
            return MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar;


        if (m_codecInfo == null) {
            Log.e(TAG, "Unable to find an appropriate codec for "
                    + VIDEO_MIME_TYPE);

            return -1;
        }

        try {
            MediaCodecInfo.CodecCapabilities capabilities = m_codecInfo
                    .getCapabilitiesForType(VIDEO_MIME_TYPE);

            MediaCodecInfo.CodecProfileLevel[] levels = capabilities.profileLevels;
            Log.e( TAG, "CodecProfileLevel" + levels.toString());

//            for (int i = 0; i < capabilities.colorFormats.length; i++) {
//                int colorFormat = capabilities.colorFormats[i];
//                Log.e( TAG, "colorFormats:[" + i + "]=" + capabilities.colorFormats[i]);
//            }


            //prefer COLOR_FormatYUV420Planar
            for (int i = 0; i < capabilities.colorFormats.length; i++) {
                int colorFormat = capabilities.colorFormats[i];

                if (MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar == colorFormat)
                    return colorFormat;
            }

            for (int i = 0; i < capabilities.colorFormats.length; i++) {
                int colorFormat = capabilities.colorFormats[i];

                if (isRecognizedFormat(colorFormat))
                    return colorFormat;
            }
        } catch (Exception e) {
            if (m_verbose)
                Log.d(TAG, "getSupportedColorFormat exception");

            return -1;
        }

        return -1;
    }

    public static boolean isInNotSupportedList() {
        return false;
    }

    /**
     * Holds state associated with a Surface used for MediaCodec encoder input.
     * <p/>
     * The constructor takes a Surface obtained from MediaCodec.createInputSurface(), and uses
     * that to create an EGL window surface.  Calls to eglSwapBuffers() cause a frame of data to
     * be sent to the video encoder.
     * <p/>
     * This object owns the Surface -- releasing this will release the Surface too.
     */
    /**
     * Code for rendering a texture onto a surface using OpenGL ES 2.0.
     */

    /**
     * Code for rendering a texture onto a surface using OpenGL ES 2.0.
     */
    public static class CodecData
    {
    	public byte[] data = null;
    	public long	pts = 0;
        public int flag;
    }


    public void startEncoderThread()
    {
        encoderThread = new Thread(new Runnable() {

            @SuppressLint("NewApi")
            @Override
            public void run() {
                isRunning = true;
                int[] input = null;
                YXLog("thread running start!!!");

                while (isRunning)
                {
                    // 初始化与重新初始化;
                    if (true == isNeedReconfigure||(configStatus&maskForceRestart)!=0)
                    {
                        if( configStatus==maskBitRate && Build.VERSION.SDK_INT>=19)// SDK_INT >= 19 支持动态码率设置，不需要重新配置编码器
                        {
                            Bundle config = new Bundle();
                            config.putInt( MediaCodec.PARAMETER_KEY_VIDEO_BITRATE, bitRate);
                            m_mediaCodec.setParameters(config);
                            configStatus = 0;
                        }
                        else
                        {
                            restartEncoder();
                        }

                        isNeedReconfigure = false;
                    }

                    // 把数据导出干净，不然数据堵上后就卡在塞数据的地方;
                    drainOutputBuffer();

                    // 这里又导出一遍，也许用不着；
                    drainOutputBuffer();

//                    if ( null == mTextureManager || mTextureManager.getCacheSize()<=0)
                    if (true)
                    {
                        // 等待罗;
                        try {
                            Thread.sleep(30);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }

                }


                closeEncoder();
                YXLog("thread running end!!!");

            }

        });
        encoderThread.start();

    }



    public void startEncoderThread2()
    {
        encoderThread = new Thread(new Runnable() {

            @SuppressLint("NewApi")
            @Override
            public void run() {
                isRunning = true;
                byte[] input;
                int inputBufferIndex = -1;
                YXLog("thread running start!!!");

                while (isRunning)
                {
                    // 初始化与重新初始化;
                    if (true == isNeedReconfigure||(configStatus&maskForceRestart)!=0)
                    {
                        if( configStatus==maskBitRate && Build.VERSION.SDK_INT>=19)// SDK_INT >= 19 支持动态码率设置，不需要重新配置编码器
                        {
                            Bundle config = new Bundle();
                            config.putInt( MediaCodec.PARAMETER_KEY_VIDEO_BITRATE, bitRate);
                            m_mediaCodec.setParameters(config);
                            configStatus = 0;
                        }
                        else
                        {
                            restartEncoder();
                        }

                        isNeedReconfigure = false;
                    }

                    // 把数据导出干净，不然数据堵上后就卡在塞数据的地方;
                    drainOutputBuffer();
                    input = inputQueue2.poll();

                    inputBufferIndex = m_mediaCodec.dequeueInputBuffer(TIMEOUT_USEC);
                    if (inputBufferIndex >= 0) {
                        long pts = computePresentationTime(m_getnerateIndex);
                        ByteBuffer inputBuffer = getInputBufferByIdx(inputBufferIndex);
                        inputBuffer.clear();
                        inputBuffer.put(input);
                        m_mediaCodec.queueInputBuffer(inputBufferIndex, 0, input.length, pts, 0);
                        ++m_getnerateIndex;
                    }

                    // 这里又导出一遍，也许用不着；
                    drainOutputBuffer();

                    if ( inputQueue2.size()<=0)
                    {
                        // 等待罗;
                        try {
                            Thread.sleep(30);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }

                }

                closeEncoder();
                YXLog("thread running end!!!");

            }

        });
        encoderThread.start();

    }

    public void stopEncoderThread()
    {
        isRunning = false;
        try {
            encoderThread.join();
        } catch (InterruptedException e)
        {
            e.printStackTrace();
        }
    }

}
