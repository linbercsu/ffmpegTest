extern "C" {
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/jni.h"
}

#include <memory>
#include <jni.h>
#include <android/log.h>
#include <GLES2/gl2.h>
#include <string>
#include <pthread.h>
#include <mutex>
#include <thread>
#include <queue>
#include <utility>
#include "libyuv.h"
#include "shader.h"
#include "GLUtil.h"
#include "BaseEffect.h"
#include "TestEffect.h"
#include "ZEffect.h"

#include "mediacodec/NXMediaCodecEncInterface.h"

extern AVCodec ff_android_hw_h264_encoder;
#define SCALE_FLAGS SWS_BICUBIC
#define STREAM_FRAME_RATE 25 /* 25 images/s */


#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    LOGI("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
                                                    = glGetError()) {
        LOGI("after %s() glError (0x%x)\n", op, error);
    }
}

/*
auto gVertexShader =
        "attribute vec4 vPosition;\n"
        "void main() {\n"
        "  gl_Position = vPosition;\n"
        "}\n";
        */
auto gVertexShader = R"(
attribute vec4 vPosition;
attribute vec2 a_TexCoordinate;
varying vec2 v_TexCoord;

void main()
{
    v_TexCoord = a_TexCoordinate;
    gl_Position = vPosition;
})";

auto gFragmentShader = R"(
precision mediump float;
uniform sampler2D u_Texture;
varying vec2 v_TexCoord;

void main()
{
    gl_FragColor = texture2D(u_Texture, v_TexCoord);
}
)";

GLint vPosition;
GLint a_TexCoordinate;
GLint u_Texture;
/*
auto gFragmentShader =
        "precision mediump float;\n"
        "void main() {\n"
        "  gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
        "}\n";
        */

const GLfloat gTriangleVertices[] = {-0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, -0.5f, -0.5f
                                     };


unsigned int indices[] = {  // note that we start from 0!
        0, 1, 2, 3  // first Triangle

};
//unsigned int indices[] = {  // note that we start from 0!
//        0, 1, 3,  // first Triangle
//        1, 2, 3   // second Triangle
//
//};

//const GLfloat gTriangleVertices[] = { 0.0f, 0.5f, -0.5f, -0.5f,
//                                      0.5f, -0.5f };

const GLfloat gTriangleTextures[] = { 0.0f, 0.0f, 1.0f, 0.0f,
                                      1.0f, 1.0f, 0.0f, 1.0f};

//const GLfloat gTriangleTextures[] = { 0.5f, 0.0f,
//                                      1.0f, 1.0f, 0.0f, 1.0f};

GLuint gProgram;
//GLuint gvPositionHandle;

bool setupGraphics(int w, int h) {
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    LOGI("setupGraphics(%d, %d)", w, h);
    gProgram = nx_effect::createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        LOGE("Could not create program.");
        return false;
    }
//    gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");
//    checkGlError("glGetAttribLocation");
//    LOGI("glGetAttribLocation(\"vPosition\") = %d\n",
//         gvPositionHandle);

//    glViewport(0, 0, w, h);
    checkGlError("glViewport");
    return true;
}


namespace hello {
    class OutputStream;

    class InputStream;

    class ProcessCallback;

    class BlockingQueue {
    public:
        void push(AVFrame* t) {
            std::unique_lock<std::mutex> lck(mutex);
            condition.wait(lck, [=]()->bool {
                return queue.size() < 3;
            });

            queue.push(t);
        }

        AVFrame* pop(int64_t pts) {
            std::unique_lock<std::mutex> lock(mutex);
            if (queue.empty())
                return nullptr;

            AVFrame* t = queue.back();
            if (t->pts > pts) {
                return nullptr;
            }

            queue.pop();
            condition.notify_one();
            return t;
        }

    private:
        std::queue<AVFrame*> queue;
        std::mutex mutex;
        std::condition_variable condition;
    };    
    
    class BlockingQueueFree {
    public:
        void push(AVFrame * t) {
            std::unique_lock<std::mutex> lck(mutex);
            queue.push(t);
            condition.notify_one();
        }

        AVFrame* pop() {
            std::unique_lock<std::mutex> lock(mutex);
            condition.wait(lock, [=]()->bool {
                return !queue.empty();
            });

            AVFrame* t = queue.back();

            queue.pop();

            return t;
        }

    private:
        std::queue<AVFrame*> queue;
        std::mutex mutex;
        std::condition_variable condition;
    };

    class GLVideo {

    public:
        static void initClass(JNIEnv *pEnv, jclass clazz);

        GLVideo(ProcessCallback *callback, const char *sourcePath);

        ~GLVideo() noexcept;

        const char *convert();

        void onSurfaceCreated();
        void onDrawFrame(int64_t time);
        void onSurfaceChanged(int w, int h);

        void onThreadRun();

        void cancel();

    private:
        static const char TAG[];

        std::unique_ptr<OutputStream> target;
        std::unique_ptr<InputStream> inputStream;
        std::unique_ptr<std::thread> thread;
    };

//const char AudioConverter::TAG[] = PROJECTIZED( "AudioConverter" );
    const char GLVideo::TAG[] = "AudioConverter";


//static jmethodID progressMethod = nullptr;
    class ProcessCallback {
    public:
        virtual ~ProcessCallback() = default;

        virtual void onProgress(int progress) = 0;
    };


    class JavaProgressCallback : public ProcessCallback {

    public:
        static void init(JNIEnv *env, jclass clazz) {

            progressMethod = env->GetMethodID(clazz, "onProgress", "(I)V");
        }

        explicit JavaProgressCallback(JNIEnv *env, jobject javaObject) :
                env(env) {
            this->javaObject = env->NewGlobalRef(javaObject);
        }


        ~JavaProgressCallback() override {
            env->DeleteGlobalRef(javaObject);
        }

        void onProgress(int progress) override {
//            if (progress > 100)
//                progress = 100;
//            if (progress > lasProgress) {
//                lasProgress = progress;
//                env->CallVoidMethod(javaObject, progressMethod, progress);
//            }
        }

    public:
        JNIEnv *env;
        jobject javaObject;
        int lasProgress = 0;
    public:
        static jmethodID progressMethod;
    };

    jmethodID JavaProgressCallback::progressMethod = nullptr;


    class ConvertException : public std::exception {
    public:
        explicit ConvertException(std::string what) : w(std::move(what)) {

        }

        explicit ConvertException(const char *what) : w(what) {
        }

        const char *what() const noexcept override {
            return w.c_str();
        }

    private:
        std::string w;
    };

    class InputStreamCallback {
    public:
        virtual void onInit() = 0;

        virtual void onAudioStream(AVCodecContext *codecContext) = 0;

        virtual void onVideoStream(AVCodecContext *codecContext, AVStream *stream) = 0;

        virtual void onStart() = 0;

        virtual void onAudioFrame(AVFrame *frame) = 0;

        virtual void onVideoFrame(AVFrame *frame) = 0;

        virtual void onEnd() = 0;
    };

    AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                               uint64_t channel_layout,
                               int sample_rate, int nb_samples) {
        AVFrame *frame = av_frame_alloc();
        int ret;

        if (!frame) {
            throw ConvertException("memory error: Error allocating an audio frame");
        }

        frame->format = sample_fmt;
        frame->channel_layout = channel_layout;
        frame->sample_rate = sample_rate;
        frame->nb_samples = nb_samples;

        if (nb_samples) {
            ret = av_frame_get_buffer(frame, 0);
            if (ret < 0) {
                throw ConvertException("memory error: Error allocating an audio buffer");
            }
        }

        return frame;
    }

    class AudioFrameBuffer {

    public:
        AVFrame* buffer = {nullptr};
        AVFrame* bufferTmp = {nullptr};
        int SIZE = 1024 * 8;
        int count = 0;

        ~AudioFrameBuffer() {
            if (buffer != nullptr) {
                av_frame_free(&buffer);
                av_frame_free(&bufferTmp);
            }
        }

        void sendFrame(AVFrame* pFrame, int pCount) {
            if (buffer == nullptr) {
                buffer = alloc_audio_frame((enum AVSampleFormat)pFrame->format, pFrame->channel_layout, pFrame->sample_rate, SIZE);
                bufferTmp = alloc_audio_frame((enum AVSampleFormat)pFrame->format, pFrame->channel_layout, pFrame->sample_rate, SIZE);
            }

            av_frame_make_writable(buffer);
            av_frame_make_writable(bufferTmp);

            if (pCount + count > SIZE) {
                throw ConvertException("buffer internal error");
            }

            memcpy(buffer->data[0] + count * 4, pFrame->data[0], pCount * 4);
            memcpy(buffer->data[1] + count * 4, pFrame->data[1], pCount * 4);
            count += pCount;
        }

        int receiveFrame(AVFrame* out, int pCount) {
            if (count < pCount)
                return AVERROR(EAGAIN);

            out->nb_samples = pCount;
            memcpy(out->data[0], buffer->data[0], pCount * 4);
            memcpy(out->data[1], buffer->data[1], pCount * 4);

            int remain = count - pCount;
            count = remain;
            if (remain == 0) {
                return 0;
            }

            memcpy(bufferTmp->data[0], buffer->data[0] + pCount * 4, remain * 4);
            memcpy(bufferTmp->data[1], buffer->data[1] + pCount * 4, remain * 4);

            memcpy(buffer->data[0], bufferTmp->data[0], remain * 4);
            memcpy(buffer->data[1], bufferTmp->data[1], remain * 4);
            return 0;
        }
    };

    class InputStream {

    public:
        explicit InputStream(ProcessCallback *processCallback, InputStreamCallback *callback,
                             const char *path) : processCallback(processCallback),
                                                 callback(callback), sourcePath(path), lockMutex() {
        }

        ~InputStream() {
            release();
        }

    private:
        void release() {
            if (audio_dec_ctx != nullptr) {
                avcodec_free_context(&audio_dec_ctx);
                audio_dec_ctx = nullptr;
            }

            if (video_dec_ctx != nullptr) {
                avcodec_free_context(&video_dec_ctx);
                video_dec_ctx = nullptr;
            }

            if (fmt_ctx != nullptr) {
                avformat_close_input(&fmt_ctx);
                fmt_ctx = nullptr;
            }

            if (pkt != nullptr) {
                av_packet_free(&pkt);
                pkt = nullptr;
            }

            if (frame != nullptr) {
                av_frame_free(&frame);
                frame = nullptr;
            }
            if (videoFrame != nullptr) {
                av_frame_free(&videoFrame);
                videoFrame = nullptr;
            }
        }

    private:
        InputStreamCallback *callback;
        AVFormatContext *fmt_ctx = nullptr;
        AVCodecContext *audio_dec_ctx = nullptr;
        AVCodecContext *video_dec_ctx = nullptr;
        int width = 0, height = 0;
        enum AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;
        AVStream *audio_stream = nullptr;
        AVStream *video_stream = nullptr;
        const char *src_filename = nullptr;
        int audio_stream_idx = -1;
        int video_stream_idx = -1;
        AVFrame *frame = nullptr;
        AVFrame *videoFrame = nullptr;
        AVPacket *pkt = nullptr;
        int audio_frame_count = 0;
        int64_t duration = 0;
        std::unique_ptr<ProcessCallback> processCallback;
        bool stopped = false;
        std::mutex lockMutex;

        bool isStopped() {
            std::lock_guard<std::mutex> lg(lockMutex);
            return stopped;
        }

        void output_audio_frame(AVFrame *audioFrame) {
            callback->onAudioFrame(audioFrame);
        }

        void output_video_frame(AVFrame *pFrame) {
//        __android_log_print(6, "GLVideo", "on video frame %ld", pFrame->pts);
            callback->onVideoFrame(pFrame);
        }

        int decode_packet(AVCodecContext *dec, const AVPacket *package) {
            int ret;

            // submit the packet to the decoder
            ret = avcodec_send_packet(dec, package);
            if (ret < 0) {

                throw ConvertException(
                        std::string("decode error: Error submitting a packet for decoding: ") +
                        av_err2str(ret));
//            fprintf(stderr, "Error submitting a packet for decoding (%s)\n", av_err2str(ret));
            }

            // get all the available frames from the decoder
            while (ret >= 0) {
                ret = avcodec_receive_frame(dec, frame);
                if (ret < 0) {
                    // those two return values are special and mean there is no output
                    // frame available, but there were no errors during decoding
                    if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                        return 0;

                    throw ConvertException(
                            std::string("decode error: Error during decoding: ") + av_err2str(ret));
//                fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret));
//                return ret;
                }

                // write the frame data to output file
                if (dec->codec->type == AVMEDIA_TYPE_VIDEO)
                    output_video_frame(frame);
                else
                    output_audio_frame(frame);

                av_frame_unref(frame);
            }

            return 0;
        }

        int decode_packet_video(AVCodecContext *dec, const AVPacket *package) {
            int ret;
            ret = avcodec_send_packet(dec, package);

//        __android_log_print(6, "GLVideo", "on video pkt %ld %d", package->pts, ret);
            if (ret != AVERROR(EAGAIN) && ret < 0) {
                throw ConvertException(
                        std::string("decode error: Error submitting a packet for decoding: ") +
                        av_err2str(ret));
            }


            while (ret == AVERROR(EAGAIN)) {
                while (true) {
                    ret = avcodec_receive_frame(dec, videoFrame);
                    if (ret < 0) {
                        if (ret == AVERROR_EOF)
                            return 0;

                        if (ret == AVERROR(EAGAIN)) {
                            ret = avcodec_send_packet(dec, package);
                            if (ret == 0)
                                break;

                            if (ret < 0 && ret != AVERROR(EAGAIN))
                                throw ConvertException(
                                        std::string("decode error: Error during decoding 1: ") +
                                        av_err2str(ret));
                            continue;
                        }

                        throw ConvertException(
                                std::string("decode error: Error during decoding: ") +
                                av_err2str(ret));
                    }

                    output_video_frame(videoFrame);

                    av_frame_unref(videoFrame);
                    ret = avcodec_send_packet(dec, package);
                    break;
                }
            }

            if (ret < 0 && ret != AVERROR(EAGAIN))
                throw ConvertException(
                        std::string("decode error: Error during decoding 2: ") +
                        av_err2str(ret));


            ret = 0;
            // get all the available frames from the decoder
            while (ret >= 0) {
                ret = avcodec_receive_frame(dec, videoFrame);
                if (ret < 0) {
                    // those two return values are special and mean there is no output
                    // frame available, but there were no errors during decoding
                    if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                        return 0;

                    throw ConvertException(
                            std::string("decode error: Error during decoding: ") + av_err2str(ret));
//                fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret));
//                return ret;
                }

                // write the frame data to output file
                output_video_frame(videoFrame);

                av_frame_unref(videoFrame);
            }

            return 0;
        }

        static void open_codec_context(int *stream_idx,
                                       AVCodecContext **dec_ctx, AVFormatContext *formatContext,
                                       enum AVMediaType type) {
            int ret, stream_index;
            AVStream *st;
            const AVCodec *dec;
            AVDictionary *opts = nullptr;

            ret = av_find_best_stream(formatContext, type, -1, -1, nullptr, 0);
            if (ret < 0) {
                return;
//            fprintf(stderr, "Could not find %s stream in input file '%s'\n",
//                    av_get_media_type_string(type), src_filename);
//            return ret;
            } else {
                stream_index = ret;
                st = formatContext->streams[stream_index];
                if (AVMEDIA_TYPE_VIDEO == type) {
                    st->codecpar->format = AV_PIX_FMT_YUV420P;

                }

                /* find decoder for the stream */
                if (st->codecpar->codec_id == AV_CODEC_ID_H264) {

                    dec = avcodec_find_decoder_by_name("h264_mediacodec");
                } else {
                    dec = avcodec_find_decoder(st->codecpar->codec_id);
                }
                if (!dec) {
                    throw ConvertException("decode error: Failed to find codec");
                }

                /* Allocate a codec context for the decoder */
                *dec_ctx = avcodec_alloc_context3(dec);
                if (!*dec_ctx) {
//                fprintf(stderr, "Failed to allocate the %s codec context\n",
//                        av_get_media_type_string(type));
                    throw ConvertException("decode error: Failed to allocated the codec context");
//                return AVERROR(ENOMEM);
                }

                /* Copy codec parameters from input stream to output codec context */
                if ((ret = (avcodec_parameters_to_context(*dec_ctx, st->codecpar))) < 0) {
//                fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
//                        av_get_media_type_string(type));
                    throw ConvertException(
                            std::string("decode error: Failed to copy codec parameters: ") +
                            av_err2str(ret));
//                return ret;
                }

                /* Init the decoders */
                if ((ret = (avcodec_open2(*dec_ctx, dec, &opts))) < 0) {
//                fprintf(stderr, "Failed to open %s codec\n",
//                        av_get_media_type_string(type));
                    throw ConvertException(
                            std::string("decode error: Failed to open codec") + av_err2str(ret));
//                return ret;
                }
                *stream_idx = stream_index;
            }

        }

    public:
        void stop() {
            std::lock_guard<std::mutex> lg(lockMutex);
            if (stopped)
                return;

            stopped = true;

        }

        void init() {
            src_filename = sourcePath.c_str();
            int ret;
            /* open input file, and allocate format context */
            if ((ret = avformat_open_input(&fmt_ctx, src_filename, nullptr, nullptr)) < 0) {
//            fprintf(stderr, "Could not open source file %s\n", src_filename);
//            exit(1);
                throw ConvertException(std::string("open source: file failed: ") + av_err2str(ret));
            }

            /* retrieve stream information */
            if ((ret = avformat_find_stream_info(fmt_ctx, nullptr)) < 0) {
//            fprintf(stderr, "Could not find stream information\n");
//            exit(1);
                throw ConvertException(
                        std::string("open source: Could not find stream information") +
                        av_err2str(ret));
            }

            callback->onInit();

            open_codec_context(&audio_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO);

            pkt = av_packet_alloc();
            if (!pkt) {
                throw ConvertException("memory error: Could not allocate packet");
            }

            if (audio_stream_idx != -1) {
                audio_stream = fmt_ctx->streams[audio_stream_idx];
                duration = audio_stream->duration;

                frame = av_frame_alloc();
                if (!frame) {
                    throw ConvertException("memory error: Could not allocate frame");
                }

                callback->onAudioStream(audio_dec_ctx);
            }

            open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO);
            if (video_stream_idx != -1) {
                videoFrame = av_frame_alloc();
                if (!videoFrame) {
                    throw ConvertException("memory error: Could not allocate frame");
                }

                video_stream = fmt_ctx->streams[video_stream_idx];

                video_dec_ctx->pkt_timebase = video_stream->time_base;

                /* allocate image where the decoded image will be put */
                width = video_dec_ctx->width;
                height = video_dec_ctx->height;
                pix_fmt = video_dec_ctx->pix_fmt;

                __android_log_print(6, "GLVideo", "find video stream %d", pix_fmt);

                callback->onVideoStream(video_dec_ctx, video_stream);
            }

            if (audio_stream_idx == -1 && video_stream_idx == -1) {
                throw ConvertException("no stream error");
            }
        }

        void start() {
            callback->onStart();
            int ret = 0;

//        if (audio_stream)
//            printf("Demuxing audio from file '%s' into '%s'\n", src_filename, audio_dst_filename);

            /* read frames from the file */
            while (!isStopped() && av_read_frame(fmt_ctx, pkt) >= 0) {
                // check if the packet belongs to a stream we are interested in, otherwise
                // skip it
//            if (pkt->stream_index == video_stream_idx)
//                ret = decode_packet(video_dec_ctx, pkt);
//            else if (pkt->stream_index == audio_stream_idx)
                if (pkt->stream_index == audio_stream_idx) {
                    ret = decode_packet(audio_dec_ctx, pkt);

                    if (ret >= 0) {
                        int progress = pkt->pts * 100 / duration;
                        processCallback->onProgress(progress);
                    }
                } else if (pkt->stream_index == video_stream_idx) {
                    ret = decode_packet_video(video_dec_ctx, pkt);

                }
                av_packet_unref(pkt);
                if (ret < 0)
                    break;

            }

            if (isStopped()) {
                throw ConvertException("cancelled");
            }

            /* flush the decoders */
            if (video_dec_ctx)
                decode_packet(video_dec_ctx, nullptr);
            if (audio_dec_ctx)
                decode_packet(audio_dec_ctx, nullptr);

//        printf("Demuxing succeeded.\n");

//        if (audio_stream) {
//            enum AVSampleFormat sfmt = audio_dec_ctx->sample_fmt;

//            __android_log_print(6, "AudioConverter", "audio %d, %d, %d, %d", audio_dec_ctx->sample_fmt, audio_dec_ctx->sample_rate, audio_dec_ctx->channels, audio_dec_ctx->channel_layout);
//            int n_channels = audio_dec_ctx->channels;
//            const char *fmt;

//            if (av_sample_fmt_is_planar(sfmt)) {
//                const char *packed = av_get_sample_fmt_name(sfmt);
//                __android_log_print(6, "AudioConverter", "planar %s", packed);
//                printf("Warning: the sample format the decoder produced is planar "
//                       "(%s). This example will output the first channel only.\n",
//                       packed ? packed : "?");
//                sfmt = av_get_packed_sample_fmt(sfmt);
//                n_channels = 1;
//            }

//            ret = get_format_from_sample_fmt(&fmt, sfmt);

//            printf("Play the output audio file with the command:\n"
//                   "ffplay -f %s -ac %d -ar %d %s\n",
//                   fmt, n_channels, audio_dec_ctx->sample_rate,
//                   audio_dst_filename);
//        }

            callback->onEnd();

            release();
        }

    private:
        std::string sourcePath;
    };

    class OutputStream : public InputStreamCallback {
    public:
        OutputStream() {
            effect = new nx_effect::ZEffect();
        }

        ~OutputStream() {
//        __android_log_write(6, "AudioConverter", "~OutputStream");
            if (codecContext != nullptr)
                avcodec_free_context(&codecContext);

            if (videoCodecContext != nullptr)
                avcodec_free_context(&videoCodecContext);

            if (frame != nullptr)
                av_frame_free(&frame);

            if (frame1 != nullptr) {
                av_frame_free(&frame1);
            }

            if (frameWrite != nullptr) {
                av_frame_free(&frameWrite);
            }

            if (videoFrame != nullptr) {
                av_frame_free(&videoFrame);
            }

            if (videoFrameRotate != nullptr) {
                av_frame_free(&videoFrame);
            }

            if (videoFrameConvert != nullptr) {
                av_frame_free(&videoFrameConvert);
            }
            if (videoFrameScale != nullptr) {
                av_frame_free(&videoFrameScale);
            }

            if (sws_ctx != nullptr) {
                sws_freeContext(sws_ctx);
                sws_ctx = nullptr;
            }
            if (swr_ctx != nullptr)
                swr_free(&swr_ctx);


            if (context != nullptr) {
                if (!(context->oformat->flags & AVFMT_NOFILE))
                    /* Close the output file. */
                    avio_closep(&context->pb);

                /* free the stream */
                avformat_free_context(context);
            }
        }

        void onCreated() {
            createTexture();
            effect->init();
        }

        void onSurfaceSizeChanged(int w, int h) {
            surfaceWidth = w;
            surfaceHeight = h;

            glViewport(w/4, h/4, w/2, h/2);
//            setupGraphics(w, h);
//            createTexture();
//            vPosition = glGetAttribLocation(gProgram, "vPosition");
//            a_TexCoordinate = glGetAttribLocation(gProgram, "a_TexCoordinate");
//            u_Texture = glGetUniformLocation(gProgram, "u_Texture");
        }
        
        void onDraw(int64_t time) {


            AVFrame *pFrame;
            int64_t pts;
            if (!firstFrameDraw) {
                pFrame = queue.pop(0x0ffffffffL);
                if (pFrame == nullptr)
                    return;

                firstFrameTime = time;
                pts = 0;
                firstFrameDraw = true;
            } else {
                pts = time - firstFrameTime;
                pFrame = queue.pop(pts);
            }

            if (pFrame != nullptr) {

                lastDrawFrame = pFrame;

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pFrame->width, pFrame->height, 0,
                                 GL_RGBA, GL_UNSIGNED_BYTE, pFrame->data[0]);
                queueFree.push(pFrame);
                glBindTexture(GL_TEXTURE_2D, 0);
            }

            glBindTexture(GL_TEXTURE_2D, texture);
            /*
            glUseProgram(gProgram);
            checkGlError("glUseProgram");

            glVertexAttribPointer(vPosition, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
            checkGlError("glVertexAttribPointer");
            glEnableVertexAttribArray(vPosition);
            checkGlError("glEnableVertexAttribArray");

            glEnableVertexAttribArray(a_TexCoordinate);
            glVertexAttribPointer(a_TexCoordinate, 2, GL_FLOAT, false, 0, gTriangleTextures);
            glUniform1i(u_Texture, 0);

//            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);

            glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, indices);
//            glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
            checkGlError("glDrawArrays");
             */
            effect->draw(pts, texture, targetWidth, targetHeight);
        }

        void createTexture() {
            auto * textures = new GLuint[2]; //生成纹理id
            glGenTextures(  //创建纹理对象
                    2, //产生纹理id的数量
                    textures
            );
            texture = textures[0];
            texture1 = textures[1];

            __android_log_print(6, "AudioConverter", "create texture %d %d", texture, texture1);

            //绑定纹理id，将对象绑定到环境的纹理单元
            glBindTexture(GL_TEXTURE_2D, texture);

            glTexParameterf(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER,GL_NEAREST);//设置MIN 采样方式
            glTexParameterf(GL_TEXTURE_2D,
                    GL_TEXTURE_MAG_FILTER,GL_LINEAR);//设置MAG采样方式
            glTexParameterf(GL_TEXTURE_2D,
                    GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);//设置S轴拉伸方式
            glTexParameterf(GL_TEXTURE_2D,
                    GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);//设置T轴拉伸方式

            //绑定纹理id，将对象绑定到环境的纹理单元
            glBindTexture(GL_TEXTURE_2D, texture1);

            glTexParameterf(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER,GL_NEAREST);//设置MIN 采样方式
            glTexParameterf(GL_TEXTURE_2D,
                    GL_TEXTURE_MAG_FILTER,GL_LINEAR);//设置MAG采样方式
            glTexParameterf(GL_TEXTURE_2D,
                    GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);//设置S轴拉伸方式
            glTexParameterf(GL_TEXTURE_2D,
                    GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);//设置T轴拉伸方式


        }


    private:
        nx_effect::BaseEffect* effect;
        int surfaceWidth;
        int surfaceHeight;


        AVFormatContext *context{};
        AVStream *stream{};
        AVCodecContext *codecContext{};
        AVCodec *encoder{};
        AVFrame *frame{};
        int frameSize{0};
        bool immediate{true};
        bool lastFrame{false};
        int lastFramePos = 0;
        AudioFrameBuffer audioFrameBuffer;
        AVFrame *frame1{};
        AVFrame *frameWrite{};
        struct SwrContext *swr_ctx{};
        int samples_count{};
        int64_t next_pts{};

        int sourceSample_rate = 0;
        uint64_t sourceLayout = 0;
        int sourceChannels = 0;

        AVStream *videoStream{};
        AVCodecContext *videoCodecContext{};
        AVCodec *videoEncoder{};
        AVFrame *videoFrame{};
        AVFrame *videoFrameScale{};
        AVFrame *videoFrameRotate{};
        AVFrame *videoFrameConvert{};
        AVFrame *videoFrameConvertRGBA{};
        std::mutex allocMutex;
        AVFrame *videoFrameConvert1{};
        AVFrame *videoFrameConvert2{};
        BlockingQueue queue;
        BlockingQueueFree queueFree;
        AVFrame * lastDrawFrame{nullptr};
        bool firstFrameDraw{false};
        int64_t firstFrameTime;
        GLuint texture;
        GLuint texture1;

        bool preRotate{false};
        AVSampleFormat sourceSampleFormat = AV_SAMPLE_FMT_NONE;
        struct SwsContext *sws_ctx{};
        int64_t next_ptsVideo{0};
        int sourceWidth = 0;
        int sourceHeight = 0;
        int targetWidth{0};
        int targetWidthTmp{0};
        int targetHeight{0};
        int targetHeightTmp{0};
        AVRational sourceVideoTimebase{};
        int64_t firstVideoPts{-1};
        bool firstVideoPtsGot{false};
        std::string sourceRotate{};
    public:
        /*
         * useless
         */
        void addAudio() {
//        context->oformat->audio_codec = AV_CODEC_ID_MP3;
            add_stream(context->oformat->audio_codec, true);

            AVDictionary *opt = nullptr;
            open_audio(opt);
        }

        void addVideo(AVCodecContext *sourceCodecContext) {
            __android_log_print(6, "AudioConverter", "addVideo %d", sourceCodecContext->pix_fmt);
            computeTargetSize(sourceCodecContext);
            if (context->oformat->video_codec == AV_CODEC_ID_MPEG4) {
                context->oformat->video_codec = AV_CODEC_ID_H264;
            }
            add_stream(context->oformat->video_codec, false);
            AVDictionary *opt = nullptr;
            open_video(opt);
        }

        void end() {
            av_frame_free(&frame);
            av_frame_free(&frame1);
            av_frame_free(&frameWrite);
            av_frame_free(&videoFrame);
            av_frame_free(&videoFrameRotate);
            av_frame_free(&videoFrameConvert);
            av_frame_free(&videoFrameScale);
//            sws_freeContext(sws_ctx);
//            swr_free(&swr_ctx);


            context = nullptr;
            codecContext = nullptr;
            frame = nullptr;
            frame1 = nullptr;
            frameWrite = nullptr;
            videoFrame = nullptr;
            videoFrameRotate = nullptr;
            videoFrameConvert = nullptr;
            videoFrameScale = nullptr;
            swr_ctx = nullptr;
            sws_ctx = nullptr;
        }

        /* Add an output stream. */
        void add_stream(enum AVCodecID codec_id, bool audio) {
            __android_log_print(6, "AudioConverter", "add stream %d", codec_id);
            int i;
            AVCodec *enc;
            /* find the encoder */
            if (codec_id == AV_CODEC_ID_H264) {
                enc = &ff_android_hw_h264_encoder;
            } else {
                enc = avcodec_find_encoder(codec_id);
            }

            if (!enc) {
                throw ConvertException("encode error: can't find encoder");
            }

            AVStream *str = avformat_new_stream(context, nullptr);
            if (!str) {
                throw ConvertException("encode error: can't new stream");
            }
            str->id = (int) (context->nb_streams - 1);
            AVCodecContext *c = avcodec_alloc_context3(enc);
            if (!c) {
                throw ConvertException("encode error: can't alloc context3");
            }
            if (audio) {
                encoder = enc;
                stream = str;
                codecContext = c;
            } else {
                videoEncoder = enc;
                videoStream = str;
                videoCodecContext = c;
            }

            switch (enc->type) {
                case AVMEDIA_TYPE_AUDIO:
                    codecContext->sample_fmt = encoder->sample_fmts ?
                                               encoder->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
                    codecContext->bit_rate = 64000;

                    codecContext->sample_rate = 44100;
                    if (encoder->supported_samplerates) {
                        codecContext->sample_rate = encoder->supported_samplerates[0];
                        for (i = 0; encoder->supported_samplerates[i]; i++) {
                            if (encoder->supported_samplerates[i] == 44100)
                                codecContext->sample_rate = 44100;
                        }
                    }
                    codecContext->channels = av_get_channel_layout_nb_channels(
                            codecContext->channel_layout);
                    codecContext->channel_layout = AV_CH_LAYOUT_STEREO;
                    if (encoder->channel_layouts) {
                        codecContext->channel_layout = encoder->channel_layouts[0];
                        for (i = 0; encoder->channel_layouts[i]; i++) {
                            if (encoder->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                                codecContext->channel_layout = AV_CH_LAYOUT_STEREO;
                        }
                    }


                    codecContext->channels = av_get_channel_layout_nb_channels(
                            codecContext->channel_layout);
                    stream->time_base = (AVRational) {1, codecContext->sample_rate};
                    break;

                case AVMEDIA_TYPE_VIDEO:
                    videoCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
                    videoCodecContext->codec_id = codec_id;

                    videoCodecContext->bit_rate =
                            (5000000 * (int64_t) targetWidth * targetHeight) / (1080 * 720);
                    /* Resolution must be a multiple of two. */
//                videoCodecContext->width    = 352;
//                videoCodecContext->height   = 288;

                    videoCodecContext->width = targetWidth;
                    videoCodecContext->height = targetHeight;

                    __android_log_print(6, "AudioConverter",
                                        "add video parameter %ld, %d, %d, %d, %d",
                                        videoCodecContext->bit_rate, targetWidth, targetHeight,
                                        sourceWidth, sourceHeight);
                    /* timebase: This is the fundamental unit of time (in seconds) in terms
                     * of which frame timestamps are represented. For fixed-fps content,
                     * timebase should be 1/framerate and timestamp increments should be
                     * identical to 1. */
//                videoStream->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
                    videoStream->time_base = sourceVideoTimebase;
                    videoCodecContext->framerate = (AVRational) {STREAM_FRAME_RATE, 1};
                    videoCodecContext->time_base = videoStream->time_base;
                    videoCodecContext->gop_size = 6; /* emit one intra frame every twelve frames at most */
                    videoCodecContext->max_b_frames = 0;
                    videoCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
                    if (videoEncoder->pix_fmts) {
                        videoCodecContext->pix_fmt = videoEncoder->pix_fmts[0];
                        for (i = 0; videoEncoder->pix_fmts[i]; i++) {
                            if (videoEncoder->pix_fmts[i] == AV_PIX_FMT_YUV420P)
                                videoCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
                        }
                    }

                    if (videoCodecContext->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
                        /* just for testing, we also add B-frames */
//                    videoCodecContext->max_b_frames = 2;
                    }
                    if (videoCodecContext->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
                        /* Needed to avoid using macroblocks in which some coeffs overflow.
                         * This does not happen with normal video, it just happens here as
                         * the motion of the chroma plane does not match the luma plane. */
//                    videoCodecContext->mb_decision = 2;
                    }
                    break;

                default:
                    break;
            }
            /* Some formats want stream headers to be separate. */
            if (context->oformat->flags & AVFMT_GLOBALHEADER)
                c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        void open_audio(AVDictionary *opt_arg) {
//        AVFormatContext *oc = context;
            AVCodec *codec = encoder;
            AVCodecContext *c;
            OutputStream *ost = this;
            int nb_samples;
            int ret;
            AVDictionary *opt = nullptr;

            c = ost->codecContext;

            /* open it */
            av_dict_copy(&opt, opt_arg, 0);
            ret = avcodec_open2(c, codec, &opt);
            av_dict_free(&opt);
            if (ret < 0) {
                throw ConvertException(std::string("encode error: Could not open audio codec: ") +
                                       av_err2str(ret));
            }

//            if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
//                nb_samples = 10000;
//            else {
//                nb_samples = c->frame_size;
//                immediate = false;
//            }
            nb_samples = 1024 * 8;

            ost->frame = getAudioFrame(1024 * 2);
//            ost->frame = alloc_audio_frame(c->sample_fmt, c->channel_layout,
//                                           c->sample_rate, nb_samples);
//
//            ost->frame1 = alloc_audio_frame(c->sample_fmt, c->channel_layout,
//                                            c->sample_rate, nb_samples);

//            ost->frameWrite = alloc_audio_frame(c->sample_fmt, c->channel_layout,
//                                                c->sample_rate, nb_samples);

            /* copy the stream parameters to the muxer */
            ret = avcodec_parameters_from_context(ost->stream->codecpar, c);
            if (ret < 0) {
                throw ConvertException(
                        std::string("encode error: Could not copy the stream parameters: ") +
                        av_err2str(ret));
            }

            /* create resampler context */
            ost->swr_ctx = swr_alloc();
            if (!ost->swr_ctx) {
                throw ConvertException("encode error: Could not allocate resampler context");
            }

            /* set options */
            av_opt_set_int(ost->swr_ctx, "in_channel_layout", sourceLayout, 0);
            av_opt_set_int(ost->swr_ctx, "out_channel_layout", c->channel_layout, 0);
            av_opt_set_int(ost->swr_ctx, "in_channel_count", sourceChannels, 0);
            av_opt_set_int(ost->swr_ctx, "out_channel_count", c->channels, 0);
            av_opt_set_int(ost->swr_ctx, "in_sample_rate", sourceSample_rate, 0);
            av_opt_set_int(ost->swr_ctx, "out_sample_rate", c->sample_rate, 0);
            av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt", sourceSampleFormat, 0);
            av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt", c->sample_fmt, 0);

            /* initialize the resampling context */
            if ((ret = (swr_init(ost->swr_ctx))) < 0) {
                throw ConvertException(
                        std::string("encode error: Failed to initialize the resampling context: ") +
                        av_err2str(ret));
            }
        }

        void open_video(AVDictionary *opt_arg) {
            int ret;
            AVCodecContext *c = videoCodecContext;
            AVCodec *codec = videoEncoder;
            AVDictionary *opt = nullptr;

//        if (codec->id == AV_CODEC_ID_H264)
//            av_opt_set(c->priv_data, "preset", "slow", 0);

            av_dict_copy(&opt, opt_arg, 0);

            /* open the codec */
            ret = avcodec_open2(c, codec, &opt);
            av_dict_free(&opt);
            if (ret < 0) {
                throw ConvertException(std::string("encode error: Could not open video codec: ") +
                                       av_err2str(ret));
            }

            /* allocate and init a re-usable frame */
//        videoFrame = alloc_picture(c->pix_fmt, targetWidthTmp, targetHeightTmp);
//        videoFrame = alloc_picture(AV_PIX_FMT_YUV420P, targetWidthTmp, targetHeightTmp);
//        videoFrameRotate = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);


            /* copy the stream parameters to the muxer */
            ret = avcodec_parameters_from_context(videoStream->codecpar, c);
            if (ret < 0) {
                throw ConvertException(
                        std::string("encode error: Could not copy the video stream parameters: ") +
                        av_err2str(ret));
            }

//        if (!sourceRotate.empty()) {
//            av_dict_set(&videoStream->metadata, "rotate", sourceRotate.c_str(), 0);
//        }

        }

        static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height) {
            AVFrame *picture;
            int ret;

            picture = av_frame_alloc();
            if (!picture)
                throw ConvertException("memory error: Error allocating an video buffer 1");

            picture->format = pix_fmt;
            picture->width = width;
            picture->height = height;

            /* allocate the buffers for the frame data */
            ret = av_frame_get_buffer(picture, 0);
            if (ret < 0) {
                throw ConvertException("memory error: Error allocating an video buffer 2");
            }

            return picture;
        }

        AVFrame *getAudioFrame(int size) {
//            __android_log_print(6, "AudioConverter", "getAudioFrame %d", size);
            if (frame == nullptr) {
                frameSize = size;
                frame = alloc_audio_frame(codecContext->sample_fmt, codecContext->channel_layout,
                                          codecContext->sample_rate, size);
            }
            if (size <= frameSize)
                return frame;

            if (frame != nullptr) {
                av_frame_free(&frame);
            }
            frame = alloc_audio_frame(codecContext->sample_fmt, codecContext->channel_layout,
                                      codecContext->sample_rate, size);

            frameSize = size;
            return frame;
        }

        /*
         * deprecated
         */
        void write_audio_frame(AVFrame *audioFrame) {
            int ret;
            int dst_nb_samples;

            if (audioFrame) {
                int r = swr_get_delay(swr_ctx, sourceSample_rate);
                dst_nb_samples = av_rescale_rnd(
                        r +
                        audioFrame->nb_samples,
                        codecContext->sample_rate, sourceSample_rate, AV_ROUND_UP);
                ret = av_frame_make_writable(frame);
                if (ret < 0)
                    throw ConvertException(
                            std::string("encode error: av_frame_make_writable error: ") +
                            av_err2str(ret));
//            exit(1);

                /* convert to destination format */
                ret = swr_convert(swr_ctx,
                                  frame->data, dst_nb_samples,
                                  (const uint8_t **) audioFrame->data, audioFrame->nb_samples);
                if (ret < 0) {
                    throw ConvertException(
                            std::string("encode error: swr_convert error: ") + av_err2str(ret));
                }

                frame->nb_samples = ret;

                if (immediate) {
                    audioFrame = frame;

                    audioFrame->pts = av_rescale_q(samples_count,
                                                   (AVRational) {1, codecContext->sample_rate},
                                                   codecContext->time_base);
                    samples_count += ret;

                    write_frame(codecContext, stream, audioFrame);
                } else {
                    if (!lastFrame) {
                        if (ret == codecContext->frame_size) {
                            audioFrame = frame;

                            audioFrame->pts = av_rescale_q(samples_count,
                                                           (AVRational) {1,
                                                                         codecContext->sample_rate},
                                                           codecContext->time_base);
                            samples_count += ret;
                            write_frame(codecContext, stream, audioFrame);
                            return;
                        }

                        lastFrame = true;
                        lastFramePos = 0;
                        AVFrame *tmp = frame1;
                        frame1 = frame;
                        frame = tmp;
                        return;
                    }

                    ret = av_frame_make_writable(frameWrite);
                    if (ret < 0)
                        throw ConvertException(
                                std::string("encode error: av_frame_make_writable 2 error: ") +
                                av_err2str(ret));

                    int nextPos = combineFrame(frame1, lastFramePos, frame, frameWrite,
                                               codecContext->frame_size);
                    if (frameWrite->nb_samples < codecContext->frame_size) {
                        lastFrame = true;
                        AVFrame *tmp = frame1;
                        frame1 = frameWrite;
                        frameWrite = tmp;
                        lastFramePos = 0;
                        return;
                    }

                    if (nextPos == frame->nb_samples) {
                        lastFrame = false;
                        lastFramePos = 0;
                    } else {
                        lastFrame = true;
                        AVFrame *tmp = frame1;
                        frame1 = frame;
                        frame = tmp;
                        lastFramePos = nextPos;
                    }

                    frameWrite->pts = av_rescale_q(samples_count,
                                                   (AVRational) {1, codecContext->sample_rate},
                                                   codecContext->time_base);

                    samples_count += frameWrite->nb_samples;
                    write_frame(codecContext, stream, frameWrite);
                }
            } else {
                write_frame(codecContext, stream, audioFrame);
            }
        }

        void write_audio_frame_immediate(AVFrame *audioFrame) {
            int ret;
            int dst_nb_samples;

            if (audioFrame) {

//            __android_log_print(6, "AudioConverter", "write audio");
                dst_nb_samples = av_rescale_rnd(
                        swr_get_delay(swr_ctx, sourceSample_rate) +
                        audioFrame->nb_samples,
                        codecContext->sample_rate, sourceSample_rate, AV_ROUND_UP);
//            __android_log_print(6, "AudioConverter", "resample %d, %d, %d, %d, %d, %d", sourceSampleFormat, codecContext->sample_fmt, sourceSample_rate, codecContext->sample_rate, audioFrame->nb_samples, dst_nb_samples);

                frame = getAudioFrame(dst_nb_samples);
                ret = av_frame_make_writable(frame);
                if (ret < 0)
                    throw ConvertException(
                            std::string("encode error: av_frame_make_writable error: ") +
                            av_err2str(ret));

                ret = swr_convert(swr_ctx,
                                  frame->data, dst_nb_samples,
                                  (const uint8_t **) audioFrame->data, audioFrame->nb_samples);
                if (ret < 0) {
                    throw ConvertException(
                            std::string("encode error: swr_convert error: ") + av_err2str(ret));
                }

                frame->nb_samples = ret;

                audioFrameBuffer.sendFrame(frame, ret);

                ret = audioFrameBuffer.receiveFrame(frame, codecContext->frame_size);
                while (ret >= 0) {
                    int64_t pts = av_rescale_q(samples_count,
                                               (AVRational) {1, codecContext->sample_rate},
                                               codecContext->time_base);

                    frame->pts = pts;
                    samples_count += frame->nb_samples;

                    write_frame(codecContext, stream, frame);
                    av_frame_make_writable(frame);
                    ret = audioFrameBuffer.receiveFrame(frame, codecContext->frame_size);
                }
            } else {
                write_frame(codecContext, stream, audioFrame);
            }
        }

        //return next position
        static int
        combineFrame(const AVFrame *frame1, int pos, const AVFrame *frame2, AVFrame *target,
                     int size) {
            int remainLast = frame1->nb_samples - pos;
            memcpy(target->data[0], frame1->data[0] + pos * 4, remainLast * 4);
            int need = size - remainLast;
            if (need > frame2->nb_samples) {
                need = frame2->nb_samples;
            }

            memcpy(target->data[0] + remainLast * 4, frame2->data[0], need * 4);

            memcpy(target->data[1], frame1->data[1] + pos * 4, remainLast * 4);
            memcpy(target->data[1] + remainLast * 4, frame2->data[1], need * 4);
            target->nb_samples = need + remainLast;
            return need;
        }

        void write_video_frame_1(AVFrame *pFrame) {
            if (!pFrame)
                return;

            if (!firstVideoPtsGot) {
                firstVideoPtsGot = true;
                firstVideoPts = pFrame->pts;
            }

            int64_t p = pFrame->pts - firstVideoPts;

            AVFrame *tempFrame = convertForScale(pFrame);
            tempFrame = scaleVideo(tempFrame);
            AVFrame *rotated = rotateVideo(tempFrame);
            AVFrame *converted = convertVideo(rotated);
            converted->pts = av_rescale_q(p, sourceVideoTimebase, {1, 1000});
            queue.push(converted);
        }

        void write_video_frame(AVFrame *pFrame) {
            if (!pFrame) {
                return;
            }

            if (!firstVideoPtsGot) {
                firstVideoPtsGot = true;
                firstVideoPts = pFrame->pts;
            }

            int64_t p = pFrame->pts - firstVideoPts;

            AVCodecContext *c = videoCodecContext;
//        __android_log_print(6, "GLVideo", "write video %d, %d", pFrame->format, c->pix_fmt);
            int ret;

            /* when we pass a frame to the encoder, it may keep a reference to it
             * internally; make sure we do not overwrite it here */
//        if ((ret = av_frame_make_writable(videoFrame)) < 0)
//            throw ConvertException(std::string("encode error: av_frame_make_writable video error: ") + av_err2str(ret));

//        if (c->pix_fmt != pFrame->format || targetWidthTmp != pFrame->width || targetHeightTmp != pFrame->height) {
            {
//            __android_log_print(6, "GLVideo", "convert for scale");
                AVFrame *tempFrame = convertForScale(pFrame);
//            __android_log_print(6, "GLVideo", "scale");
                tempFrame = scaleVideo(tempFrame);


//            if (sourceRotate.empty()) {
//                videoFrame->pts = av_rescale_q(p, sourceVideoTimebase, c->time_base);
//            __android_log_print(6, "AudioConverter", "video pts: %ld, %ld, %ld, %ld, %d, %d, %d, %d", videoFrame->pts, p, pFrame->pts, firstVideoPts, c->time_base.num, c->time_base.den, sourceVideoTimebase.num, sourceVideoTimebase.den);
//                write_frame(videoCodecContext, videoStream, tempFrame);
//            } else {
//                __android_log_print(6, "AudioConverter", "rotate");
                AVFrame *rotated = rotateVideo(tempFrame);
//                __android_log_print(6, "AudioConverter", "convert");
                AVFrame *converted = convertVideo(rotated);
//                __android_log_print(6, "AudioConverter", "convert end");
                converted->pts = av_rescale_q(p, sourceVideoTimebase, c->time_base);

//            __android_log_print(6, "AudioConverter", "video pts: %ld, %d, %d, %d, %d", p, sourceVideoTimebase.num, sourceVideoTimebase.den, c->time_base.num, c->time_base.den);
                write_frame(videoCodecContext, videoStream, converted);
//            }

            }
//        else {

//            pFrame->pts = av_rescale_q(p, sourceVideoTimebase, c->time_base);
//            write_frame(videoCodecContext, videoStream, pFrame);
//        }
        }

        AVFrame *convertForScale(AVFrame *pFrame) {
            if (pFrame->format == AV_PIX_FMT_YUV420P)
                return pFrame;

            int ret;
            if (videoFrame == nullptr) {
                videoFrame = alloc_picture(AV_PIX_FMT_YUV420P, pFrame->width, pFrame->height);

                if ((ret = av_frame_make_writable(videoFrame)) < 0)
                    throw ConvertException(
                            std::string("encode error: av_frame_make_writable video error: ") +
                            av_err2str(ret));

            }

            if (AV_PIX_FMT_YUV422P == pFrame->format) {
                libyuv::I422ToI420(
                        pFrame->data[0], pFrame->linesize[0],
                        pFrame->data[1], pFrame->linesize[1],
                        pFrame->data[2], pFrame->linesize[2],

                        videoFrame->data[0], videoFrame->linesize[0],
                        videoFrame->data[1], videoFrame->linesize[1],
                        videoFrame->data[2], videoFrame->linesize[2],

                        pFrame->width,
                        pFrame->height
                );

                return videoFrame;
            } else if (AV_PIX_FMT_YUV444P == pFrame->format) {
                libyuv::I444ToI420(
                        pFrame->data[0], pFrame->linesize[0],
                        pFrame->data[1], pFrame->linesize[1],
                        pFrame->data[2], pFrame->linesize[2],

                        videoFrame->data[0], videoFrame->linesize[0],
                        videoFrame->data[1], videoFrame->linesize[1],
                        videoFrame->data[2], videoFrame->linesize[2],

                        pFrame->width,
                        pFrame->height
                );

                return videoFrame;
            } else if (AV_PIX_FMT_NV12 == pFrame->format) {
                libyuv::NV12ToI420(
                        pFrame->data[0], pFrame->linesize[0],
                        pFrame->data[1], pFrame->linesize[1],

                        videoFrame->data[0], videoFrame->linesize[0],
                        videoFrame->data[1], videoFrame->linesize[1],
                        videoFrame->data[2], videoFrame->linesize[2],

                        pFrame->width,
                        pFrame->height
                );

                return videoFrame;
            } else if (AV_PIX_FMT_NV21 == pFrame->format) {
                libyuv::NV21ToI420(
                        pFrame->data[0], pFrame->linesize[0],
                        pFrame->data[1], pFrame->linesize[1],

                        videoFrame->data[0], videoFrame->linesize[0],
                        videoFrame->data[1], videoFrame->linesize[1],
                        videoFrame->data[2], videoFrame->linesize[2],

                        pFrame->width,
                        pFrame->height
                );

                return videoFrame;
            } else if (AV_PIX_FMT_YUYV422 == pFrame->format) {
                libyuv::YUY2ToI420(
                        pFrame->data[0], pFrame->linesize[0],

                        videoFrame->data[0], videoFrame->linesize[0],
                        videoFrame->data[1], videoFrame->linesize[1],
                        videoFrame->data[2], videoFrame->linesize[2],

                        pFrame->width,
                        pFrame->height
                );

                return videoFrame;
            } else if (AV_PIX_FMT_UYVY422 == pFrame->format) {
                libyuv::UYVYToI420(
                        pFrame->data[0], pFrame->linesize[0],

                        videoFrame->data[0], videoFrame->linesize[0],
                        videoFrame->data[1], videoFrame->linesize[1],
                        videoFrame->data[2], videoFrame->linesize[2],

                        pFrame->width,
                        pFrame->height
                );

                return videoFrame;
            }

            /* as we only generate a YUV420P picture, we must convert it
             * to the codec pixel format if needed */
            if (!sws_ctx) {
                sws_ctx = sws_getContext(pFrame->width, pFrame->height,
                                         (enum AVPixelFormat) pFrame->format,
                                         pFrame->width, pFrame->height,
                                         (enum AVPixelFormat) videoFrame->format,
                                         SCALE_FLAGS, nullptr, nullptr, nullptr);
                if (!sws_ctx) {
                    throw ConvertException("Could not initialize the sws conversion context");
                }
            }

//            __android_log_print(6, "AudioConverter", "ffmpeg convert");
            sws_scale(sws_ctx, pFrame->data,
                      pFrame->linesize, 0, pFrame->height, videoFrame->data,
                      videoFrame->linesize);

            return videoFrame;
        }

        AVFrame *scaleVideo(AVFrame *pFrame) {
            int ret;
            if (videoFrameScale == nullptr) {
                videoFrameScale = alloc_picture(AV_PIX_FMT_YUV420P, targetWidthTmp,
                                                targetHeightTmp);
            }
            if ((ret = av_frame_make_writable(videoFrameScale)) < 0)
                throw ConvertException(
                        std::string("encode error: av_frame_make_writable video scale error: ") +
                        av_err2str(ret));

            libyuv::FilterMode mode = libyuv::kFilterBox;
            libyuv::I420Scale(pFrame->data[0], pFrame->linesize[0],
                              pFrame->data[1], pFrame->linesize[1],
                              pFrame->data[2], pFrame->linesize[2],
                              pFrame->width, pFrame->height,

                              videoFrameScale->data[0], videoFrameScale->linesize[0],
                              videoFrameScale->data[1], videoFrameScale->linesize[1],
                              videoFrameScale->data[2], videoFrameScale->linesize[2],
                              videoFrameScale->width,
                              videoFrameScale->height,
                              mode
            );

            return videoFrameScale;
        }

        /*
         * now we only support AV_PIX_FMT_YUV420P rotate
         */
        AVFrame *rotateVideo(AVFrame *pFrame) {
            if (sourceRotate.empty())
                return pFrame;
            libyuv::RotationModeEnum mode = {};
            if (sourceRotate == "90") {
                mode = libyuv::kRotate90;
            } else if (sourceRotate == "180") {
                mode = libyuv::kRotate180;
            } else if (sourceRotate == "270") {
                mode = libyuv::kRotate270;
            } else {
                return pFrame;
            }

            if (pFrame->format == AV_PIX_FMT_YUV420P) {
                if (videoFrameRotate == nullptr)
                    videoFrameRotate = alloc_picture(AV_PIX_FMT_YUV420P, targetWidth,
                                                     targetHeight);

                if ((av_frame_make_writable(videoFrameRotate)) < 0)
                    throw ConvertException(std::string(
                            "encode error: av_frame_make_writable rotate video error: "));


                libyuv::I420Rotate(pFrame->data[0], pFrame->linesize[0],
                                   pFrame->data[1], pFrame->linesize[1],
                                   pFrame->data[2], pFrame->linesize[2],

                                   videoFrameRotate->data[0], videoFrameRotate->linesize[0],
                                   videoFrameRotate->data[1], videoFrameRotate->linesize[1],
                                   videoFrameRotate->data[2], videoFrameRotate->linesize[2],

                                   pFrame->width,
                                   pFrame->height,
                                   mode
                );
            } else {
                return pFrame;
            }

            return videoFrameRotate;
        }

        AVFrame *convertVideo(AVFrame *pFrame) {
            if (videoFrameConvert == nullptr) {
                videoFrameConvert = alloc_picture(AV_PIX_FMT_ARGB, targetWidth,
                                                  targetHeight);

                videoFrameConvertRGBA = alloc_picture(AV_PIX_FMT_RGBA, targetWidth,
                                                  targetHeight);
                videoFrameConvert1 = alloc_picture(AV_PIX_FMT_RGBA, targetWidth,
                                                  targetHeight);
                videoFrameConvert2 = alloc_picture(AV_PIX_FMT_RGBA, targetWidth,
                                                  targetHeight);

                av_frame_make_writable(videoFrameConvert1);
                av_frame_make_writable(videoFrameConvert2);

                queueFree.push(videoFrameConvert1);
                queueFree.push(videoFrameConvert2);
            }



            if ((av_frame_make_writable(videoFrameConvert)) < 0)
                throw ConvertException(
                        std::string("encode error: av_frame_make_writable convert video error: "));


            libyuv::I420ToARGB(pFrame->data[0], pFrame->linesize[0],
                               pFrame->data[1], pFrame->linesize[1],
                               pFrame->data[2], pFrame->linesize[2],

                               videoFrameConvert->data[0], videoFrameConvert->linesize[0],
                               pFrame->width,
                               pFrame->height

            );

            AVFrame *pAvFrame = queueFree.pop();
            libyuv::ARGBToABGR(videoFrameConvert->data[0], videoFrameConvert->linesize[0],
                               pAvFrame->data[0], pAvFrame->linesize[0],
                               targetWidth,
                               targetHeight);
//            libyuv::ARGBCopy(videoFrameConvert->data[0], videoFrameConvert->linesize[0],
//                             pAvFrame->data[0], pAvFrame->linesize[0],
//                             targetWidth,
//                             targetHeight);

//            libyuv::ARGBToRGBA(videoFrameConvert->data[0], videoFrameConvert->linesize[0],
//                               pAvFrame->data[0], pAvFrame->linesize[0],
//                               targetWidth, targetHeight
//                               );

            return pAvFrame;

            /*
            std::unique_lock<std::mutex> lock(allocMutex);
            AVFrame* frameTmp = alloc_picture(AV_PIX_FMT_RGBA, targetWidth,
                                               targetHeight);

            av_frame_make_writable(frameTmp);
            libyuv::ARGBCopy(videoFrameConvert1->data[0], videoFrameConvert1->linesize[0],
                             frameTmp->data[0], frameTmp->linesize[0],
                             targetWidth,
                             targetHeight);

            return frameTmp;
             */
        }

        void write_frame(AVCodecContext *c, AVStream *st, AVFrame *pFrame) {
            AVFormatContext *fmt_ctx = context;
            int ret = 0;


            // send the frame to the encoder
            ret = avcodec_send_frame(c, pFrame);
            if (ret < 0) {
                throw ConvertException(
                        std::string("encode error: Error sending a video frame to the encoder: ") +
                        av_err2str(ret));
            }

            while (ret >= 0) {
                AVPacket pkt = {nullptr};

                ret = avcodec_receive_packet(c, &pkt);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                else if (ret < 0) {
                    throw ConvertException(std::string("encode error: Error encoding a frame: ") +
                                           av_err2str(ret));
                }

//            __android_log_print(6, "AudioConverter", "video pts: %ld, %d, %d", pkt.pts, c->time_base.num, c->time_base.den);
                /* rescale output packet timestamp values from codec to stream timebase */
                av_packet_rescale_ts(&pkt, c->time_base, st->time_base);
                pkt.stream_index = st->index;

                /* Write the compressed frame to the media file. */
//        log_packet(fmt_ctx, &pkt);
                ret = av_interleaved_write_frame(fmt_ctx, &pkt);
                av_packet_unref(&pkt);
                if (ret < 0) {
                    throw ConvertException(
                            std::string("encode error: av_interleaved_write_frame: ") +
                            av_err2str(ret));
                }
            }

        }

        static int computeSize(int s, int max) {
            while (s > max) {
                s = s - 64;
            }

            return s;
        }

        static int normalizeSize(int s) {
            int y = s % 64;
            if (y == 0)
                return s;

            int r = s / 64;
            int ret = 64 * (r - 1);
            if (ret <= 0)
                return 64;
            else
                return ret;
        }

        void computeTargetSize(AVCodecContext *sourceCodecContext) {
            int w = sourceCodecContext->width;
            int h = sourceCodecContext->height;

            if (w < h) {
                w = sourceCodecContext->height;
                h = sourceCodecContext->width;
            }

            int cW = computeSize(w, 1080);
            int cH = computeSize(h, 720);

            float scaleW, scaleH;
            scaleW = (float) cW / (float) w;
            scaleH = (float) cH / (float) h;

            float scale = scaleW < scaleH ? scaleW : scaleH;
            targetWidth = normalizeSize((int) (w * scale));
            targetHeight = normalizeSize((int) (h * scale));

            if (sourceCodecContext->width < sourceCodecContext->height) {
                int temp = targetWidth;
                targetWidth = targetHeight;
                targetHeight = temp;
            }

            targetWidthTmp = targetWidth;
            targetHeightTmp = targetHeight;

            if (sourceRotate == "90" || sourceRotate == "270") {
                int temp = targetWidth;
                targetWidth = targetHeight;
                targetHeight = temp;
            }
        }

        void onInit() override {
            int ret;
            //todo
        }

        void onAudioStream(AVCodecContext *sourceCodecContext) override {
            sourceSample_rate = sourceCodecContext->sample_rate;
            sourceLayout = sourceCodecContext->channel_layout;
            sourceChannels = sourceCodecContext->channels;
            sourceSampleFormat = sourceCodecContext->sample_fmt;
            __android_log_print(6, "AudioConverter", "onAudioStream %d, %d", sourceSample_rate,
                                sourceSampleFormat);

//            addAudio();

        }

        void onVideoStream(AVCodecContext *sourceCodecContext, AVStream *st) override {
            sourceWidth = sourceCodecContext->width;
            sourceHeight = sourceCodecContext->height;
            sourceVideoTimebase = sourceCodecContext->pkt_timebase;
            AVDictionaryEntry *pEntry = av_dict_get(st->metadata, "rotate", nullptr,
                                                    AV_DICT_MATCH_CASE);
            if (pEntry != nullptr) {
                sourceRotate = pEntry->value;
            }
            __android_log_print(6, "AudioConverter", "onVideoStream %d, %d, %d, %d", sourceWidth,
                                sourceHeight, sourceCodecContext->time_base.num,
                                sourceCodecContext->time_base.den);
            computeTargetSize(sourceCodecContext);
//            addVideo(sourceCodecContext);
        }

        void onStart() override {
            int ret;

        }

        void onAudioFrame(AVFrame *audioFrame) override {
//            write_audio_frame_immediate(audioFrame);
//            write_audio_frame(audioFrame);
        }

        void onVideoFrame(AVFrame *pFrame) override {
//            __android_log_print(6, "GLVideo", "onVideoFrame %d, %d, %d", pFrame->format, pFrame->width, pFrame->height);
            write_video_frame_1(pFrame);
        }

        void onEnd() override {
            end();
        }

    };


    GLVideo::GLVideo(ProcessCallback *callback, const char *sourcePath)
            : target(new OutputStream()),
              inputStream(new InputStream(callback, target.get(), sourcePath)) {

    }

    GLVideo::~GLVideo() noexcept = default;


    void GLVideo::onSurfaceCreated() {
        target->onCreated();
    }

    void GLVideo::onThreadRun() {
        inputStream->init();
        inputStream->start();
    }

    void GLVideo::onDrawFrame(int64_t time) {
        target->onDraw(time);
    }

    void GLVideo::onSurfaceChanged(int w, int h) {
        target->onSurfaceSizeChanged(w, h);

//        inputStream->init();
//        thread.reset(new std::thread(&GLVideo::onThreadRun, this));
    }


    const char *GLVideo::convert() {
        try {
            inputStream->init();
            inputStream->start();
//        inputStream.reset();
//        target.reset();
        } catch (std::exception &e) {
//        __android_log_write(6, "AudioConverter", e.what());
            return e.what();
        }

        return nullptr;
    }

    void GLVideo::cancel() {
        inputStream->stop();
    }


//////////////jni

    jlong nativeInit(JNIEnv *env,
                     jobject thzz, jstring source) {

        jboolean copy;
        const char *sourcePath = env->GetStringUTFChars(source, &copy);

        auto *progressCallback = new JavaProgressCallback(env, thzz);
        auto *converter = new GLVideo(progressCallback, sourcePath);

        env->ReleaseStringUTFChars(source, sourcePath);

        return jlong(converter);
    }

    void nativeOnSurfaceCreated(JNIEnv *env,
                                jobject  /*thzz*/, jlong ptr) {
        auto *converter = reinterpret_cast<GLVideo *>(ptr);
        converter->onSurfaceCreated();
    }

    void nativeOnDrawFrame(JNIEnv *env,
                                jobject  /*thzz*/, jlong ptr, jlong time) {
        auto *converter = reinterpret_cast<GLVideo *>(ptr);
        converter->onDrawFrame(time);
    }
    void nativeOnSurfaceChanged(JNIEnv *env,
                                jobject  /*thzz*/, jlong ptr, jint w, jint h) {
        auto *converter = reinterpret_cast<GLVideo *>(ptr);
        converter->onSurfaceChanged(w, h);
    }

    jstring nativeConvert(JNIEnv *env,
                          jobject  /*thzz*/, jlong ptr) {
        /*
        auto *converter = reinterpret_cast<GLVideo *>(ptr);
        const char *error = converter->convert();
        if (error == nullptr)
            return nullptr;

        return env->NewStringUTF(error);
         */
    }

    void nativeStop(JNIEnv * /*env*/,
                    jobject  /*thzz*/, jlong ptr) {
        auto *converter = reinterpret_cast<GLVideo *>(ptr);
//        converter->cancel();
    }

    void nativeRelease(JNIEnv * /*env*/,
                       jobject  /*thzz*/, jlong ptr) {
        auto *converter = reinterpret_cast<GLVideo *>(ptr);
        delete converter;
    }

    void nativeRun(JNIEnv * /*env*/,
                       jobject  /*thzz*/, jlong ptr) {
        auto *converter = reinterpret_cast<GLVideo *>(ptr);
        converter->onThreadRun();
    }

    void av_log_my_callback(void *ptr, int level, const char *fmt, va_list vl) {
        AVClass *avc = ptr ? *(AVClass **) ptr : nullptr;
        __android_log_print(6, avc ? avc->class_name : "GLVideo", fmt, vl);
    }

    const JNINativeMethod methods[] =
            {
                    {"nativeInit",    "(Ljava/lang/String;)J", (void *) nativeInit},
                    {"nativeRelease", "(J)V",                                                      (void *) nativeRelease},
                    {"nativeRun", "(J)V",                                                      (void *) nativeRun},
                    {"nativeOnSurfaceChanged", "(JII)V",                                     (void *) nativeOnSurfaceChanged},
                    {"nativeOnDrawFrame",    "(JJ)V",                                                      (void *) nativeOnDrawFrame},
                    {"nativeOnSurfaceCreated", "(J)V",                                                      (void *) nativeOnSurfaceCreated},
            };

    void GLVideo::initClass(JNIEnv *env, jclass clazz) {
//    av_log_set_callback(&av_log_my_callback);
        env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0]));
        JavaProgressCallback::init(env, clazz);
        JavaVM *vm = nullptr;
        env->GetJavaVM(&vm);
        jint version = env->GetVersion();
        av_jni_set_java_vm(vm, nullptr);
        __android_log_print(6, "GLVideo", "initClass %d", version);
//        YX_AMediaCodec_Enc_loadClassEnv(vm, version);
//    JavaProgressCallback::progressMethod = env->GetMethodID(clazz, "onProgress", "(I)V");
//    if ( env->ExceptionCheck() )
//    {
//        _env->ExceptionDescribe();
//    }
    }


    extern "C" JNIEXPORT void JNICALL
    Java_com_mxtech_av_GLVideo_nativeInitClass(
            JNIEnv* env,
            jclass clazz) {
        GLVideo::initClass(env, clazz);
    }

}






