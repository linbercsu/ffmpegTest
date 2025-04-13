//
// Created by linlin zhao on 2024/5/23.
//

#include "VideoRender.h"
#include "VideoPackageQueue.h"
#include "Exception.h"
#include <thread>
#include <chrono>
#include "Log.h"
#include <exception>
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>
#include "GLUtil.h"
#include "libyuv.h"
#include "ZEffect.h"
#include "GrayEffect.h"
#include "MirrorEffect.h"
#include "Releasable.h"
#include "define.h"

#include "lodepng.h"
#include <iostream>
#include <chrono>


//#include <android/bitmap.h>

extern "C" {
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

using namespace std::chrono;
#define SCALE_FLAGS SWS_BICUBIC

namespace next {
    bool lastRender = false;

    namespace {
        const int MESSAGE_ID_RENDER_IDLE = Message::MESSAGE_ID_USER + 1;
        const int MESSAGE_ID_INIT_RENDER = MESSAGE_ID_RENDER_IDLE + 1;
        const int MESSAGE_ID_INIT_DECODER = MESSAGE_ID_INIT_RENDER + 1;
        const int MESSAGE_ID_PROCESS_PACKAGE = MESSAGE_ID_INIT_DECODER + 1;
        const int MESSAGE_ID_RESEND_PACKAGE = MESSAGE_ID_PROCESS_PACKAGE + 1;
        const int MESSAGE_ID_RECEIVE_FRAME = MESSAGE_ID_RESEND_PACKAGE + 1;
        const int MESSAGE_ID_PRE_SEEK = MESSAGE_ID_RECEIVE_FRAME + 1;

        const int MESSAGE_PRIORITY_RENDER = Message::MESSAGE_PRIORITY_NORMAL + 1;
        const int MESSAGE_PRIORITY_PRE_SEEK = Message::MESSAGE_PRIORITY_NORMAL + 1;
        const int MESSAGE_PRIORITY_INIT = MESSAGE_PRIORITY_PRE_SEEK + 1;


    }

    namespace {
        int64_t nowMicro() {
            microseconds ms = duration_cast< microseconds >(
                    system_clock::now().time_since_epoch()
            );

            return ms.count();
        }

        void saveImage(const char* filename, const unsigned char
        *image, unsigned width, unsigned height) {
            //Encode the image
            lodepng_encode32_file(filename, image, width, height);
        }

        nx_effect::BaseEffect* createEffect(int effectIndex, int rotation) {
            nx_effect::BaseEffect *effect;
            if (effectIndex == 1) {
                effect = new nx_effect::MirrorEffect(rotation);
            } else if (effectIndex == 2) {
                effect = new nx_effect::GrayEffect();
            } else {
                effect = new nx_effect::ZEffect();
            }

            effect->init();

            return effect;
        }

        int getCorrectWidth(AVFrame * frame) {
            return frame->width;

            /*
            if (frame->width < 64) {
                return 64;
            }

            if (frame->width < 128) {
                return 128;
            }

            if (frame->width < 256) {
                return 256;
            }

            if (frame->width < 512) {
                return 512;
            }

            if (frame->width < 1024) {
                return 1024;
            }

            if (frame->width < 2048) {
                return 2048;
            }

            return 4096;
             */
        }

        GLuint createTexture() {
            GLuint texture;
            glGenTextures(  //创建纹理对象
                    1, //产生纹理id的数量
                    &texture
            );

//        __android_log_print(6, "AudioConverter", "create texture %d %d", texture, texture1);

            glBindTexture(GL_TEXTURE_2D, texture);

            glTexParameterf(GL_TEXTURE_2D,
                            GL_TEXTURE_MIN_FILTER, GL_LINEAR);//设置MIN 采样方式
            glTexParameterf(GL_TEXTURE_2D,
                            GL_TEXTURE_MAG_FILTER, GL_LINEAR);//设置MAG采样方式
            glTexParameterf(GL_TEXTURE_2D,
                            GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);//设置S轴拉伸方式
            glTexParameterf(GL_TEXTURE_2D,
                            GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);//设置T轴拉伸方式


            return texture;
        }
    }

    class DataContext {
    public:
        ~DataContext() {
            if (decoderContext != nullptr) {
                avcodec_free_context(&decoderContext);
                decoderContext = nullptr;
            }
        }
        AVCodecContext* decoderContext{nullptr};
        AVCodec* decoder{nullptr};
        AVPacket* resendPkt{nullptr};
        bool seek{false};
    };

    static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height) {
        AVFrame *picture;
        int ret;

        picture = av_frame_alloc();
        if (!picture)
            throw std::bad_alloc();

        picture->format = pix_fmt;
        picture->width = width;
        picture->height = height;

        /* allocate the buffers for the frame data */
        ret = av_frame_get_buffer(picture, 0);
        if (ret < 0) {
            throw std::bad_alloc();
        }

        return picture;
    }

    AVFrame *convert(AVFrame *pFrame) {
//        next_log("convert w:%d h:%d, %d", pFrame->width, pFrame->height, __LINE__);
        auto newWidth = getCorrectWidth(pFrame);
        auto videoFrameConvertRGBA = alloc_picture(AV_PIX_FMT_RGBA, newWidth,
                                                   pFrame->height);
        //just use a useless field 'channels' to remember the padding.
        videoFrameConvertRGBA->channels = (newWidth - pFrame->width) / 2;

        av_frame_make_writable(videoFrameConvertRGBA);
/*
        struct SwsContext *sws_ctx = nullptr;
            sws_ctx = sws_getContext(pFrame->width, pFrame->height,
                                     (enum AVPixelFormat) pFrame->format,
                                     pFrame->width, pFrame->height,
                                     AV_PIX_FMT_RGBA,
                                     SCALE_FLAGS, nullptr, nullptr, nullptr);

        sws_scale(sws_ctx, pFrame->data,
                  pFrame->linesize, 0, pFrame->height, videoFrameConvertRGBA->data,
                  videoFrameConvertRGBA->linesize);

        sws_freeContext(sws_ctx);

*/
        libyuv::I420ToABGR(pFrame->data[0], pFrame->linesize[0],
                           pFrame->data[1], pFrame->linesize[1],
                           pFrame->data[2], pFrame->linesize[2],

                           videoFrameConvertRGBA->data[0], videoFrameConvertRGBA->linesize[0],
                           pFrame->width,
                           pFrame->height
        );

        //std::string base = "/sdcard/Download/";
//        saveImage((base + std::to_string(pFrame->pts) + ".png").c_str(), videoFrameConvertRGBA->data[0], videoFrameConvertRGBA->width, videoFrameConvertRGBA->height);


        if (newWidth != pFrame->width) {//align real image center
            auto frameTemp = alloc_picture(AV_PIX_FMT_RGBA, newWidth,
                                                       pFrame->height);
            av_frame_make_writable(frameTemp);
            frameTemp->channels = videoFrameConvertRGBA->channels;

            int height = pFrame->height;
            auto padding = 4 * (newWidth - pFrame->width) / 2;
            auto stripe = videoFrameConvertRGBA->linesize[0];
            auto count = pFrame->width * 4;
            for (int i = 0; i < height; i++) {
                std::memcpy((frameTemp->data[0]) + (stripe * i) + padding, (videoFrameConvertRGBA->data[0]) + (stripe * i), count);
            }

            av_frame_free(&videoFrameConvertRGBA);
            videoFrameConvertRGBA = frameTemp;
        }

        
        return videoFrameConvertRGBA;
    }

    /////////////////////////////////

    RenderThread::RenderThread(): mThread(this) {
        mThread.messageQueue().pushBack(Message::simpleMessage(MESSAGE_ID_INIT_RENDER).withCallback(this));
    }

    void RenderThread::handleMessage(const next::Message &message) {
        auto id = message.getId();
        if (id == MESSAGE_ID_INIT_RENDER) {
            mThread.messageQueue().pushBack(Message::simpleMessage(MESSAGE_ID_INIT_RENDER).priority(MESSAGE_PRIORITY_RENDER).delay(16).withCallback(this));

            render();
            mThread.messageQueue().pushBack(Message::simpleMessage(MESSAGE_ID_RENDER_IDLE).withCallback(this));
        } else if (id == MESSAGE_ID_RENDER_IDLE) {
            onIdle();
        }
    }

    void RenderThread::render() {

    }

    void RenderThread::onIdle() {

    }

    void RenderThread::stop() {
        mThread.stop();
    }

    void RenderThread::join() {
        mThread.join();
    }

    void RenderThread::onThreadEnded() {}

    VideoRender::VideoRender(next::VideoPackageQueue *pQueue, MediaClock* clock) : mThread(this), mQueueRef(pQueue),
                                                                mFrameQueue(1024 * 1024 * 250), mClock(clock) {
//        mThread = new std::thread(&VideoRender::run, this);
//        effect = createEffect(mEffectIndex);        sendMessage(MESSAGE_ID_OPEN);
        sendMessage(MESSAGE_ID_PROCESS_PACKAGE);
    }

    VideoRender::~VideoRender() {
        next_log_tag("video", "delete VideoRender %d", __LINE__);
//        delete mThread;
//        mThread = nullptr;

        if (effect != nullptr) {
            delete effect;
            effect = nullptr;
        }
    }

    void VideoRender::release() {
        mFrameQueue.clear();
        if (mCurrentFrame != nullptr) {
            av_frame_free(&mCurrentFrame);
            mCurrentFrame = nullptr;
        }

        if (reusedVideoFrame != nullptr) {
            av_frame_free(&reusedVideoFrame);
            reusedVideoFrame = nullptr;
        }

        if (mDataContext != nullptr) {
            delete mDataContext;
            mDataContext = nullptr;
        }

        glDeleteTextures(1, textures);
    }

    void VideoRender::handleMessage(const next::Message &message) {
//        next_log("handleMessage %d, %d", message.getId(), __LINE__);

        switch (message.getId()) {
            case MESSAGE_ID_INIT_DECODER: {
                onMessageInit();
                break;
            }

            case MESSAGE_ID_PROCESS_PACKAGE: {
                onMessageProcessPkt();
                break;
            }

            case MESSAGE_ID_RESEND_PACKAGE: {
                onMessageResendPkt();
                break;
            }

            case MESSAGE_ID_RECEIVE_FRAME: {
                onMessageReceiveFrame();
                break;
            }

            case MESSAGE_ID_PRE_SEEK: {
//                onMessagePreSeek();
                break;
            }

            default:{
                break;
            }
        }
    }

    void VideoRender::onThreadEnded() {
        release();
    }

    void VideoRender::onMessageInit() {
        int ret = 0;

        AVCodecParameters *codecParameters = nullptr;
        AVRational timeBase;
        int baseRotation = 0;
        codecParameters = mQueueRef->getCodecParameters();

        if (codecParameters == nullptr) {
            sendMessageDelay(MESSAGE_ID_INIT_DECODER, 10);
            return;
        }

        if (mDataContext != nullptr) {
            delete mDataContext;
            mDataContext = nullptr;
        }

        mDataContext = new DataContext();

        timeBase = mQueueRef->getTimebase();
        baseRotation = mQueueRef->getRotation();

        mBaseRotation = baseRotation;
        auto decoder = avcodec_find_decoder(codecParameters->codec_id);
        mDataContext->decoderContext = avcodec_alloc_context3(decoder);
        auto dec_ctx = mDataContext->decoderContext;

        next_log_tag("video", "info: codec id %d, source f:%d %d", codecParameters->codec_id, codecParameters->format, __LINE__);

        ret = avcodec_parameters_to_context(dec_ctx, codecParameters);

        dec_ctx->pkt_timebase = timeBase;
        dec_ctx->thread_count = 16;  // specific number
        dec_ctx->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;  // both
        /*
        dec_ctx->lowres = 1;  // Half resolution
        dec_ctx->flags2 |= AV_CODEC_FLAG2_FAST;
        dec_ctx->skip_loop_filter = AVDISCARD_NONREF;
        */
        AVDictionary *opts = nullptr;
        ret = avcodec_open2(dec_ctx, decoder, &opts);
        mDataContext->decoder = decoder;

        reusedVideoFrame = av_frame_alloc();

        sendMessage(MESSAGE_ID_PROCESS_PACKAGE);
    }

    void VideoRender::onMessageProcessPkt() {
        bool clear = false;
        int ret = 0;

        AVPacket *pkt = mQueueRef->getPkt(&clear);
        if (pkt == nullptr) {
            sendMessageDelay(MESSAGE_ID_PROCESS_PACKAGE, 10);
            return;
        }

        if (pkt->stream_index == NEXT_INDEX_TRACK_CHANGED) {
            av_packet_free(&pkt);
            sendMessage(MESSAGE_ID_INIT_DECODER, MESSAGE_PRIORITY_INIT);
            return;
        } else if (pkt->stream_index == NEXT_INDEX_END) {
            av_packet_free(&pkt);
            pkt = nullptr;

            mDataContext->resendPkt = nullptr;
            onMessageResendPkt();
        } else if (pkt->stream_index == NEXT_INDEX_SEEK) {
            mDataContext->seek = false;
            std::lock_guard<std::mutex> l(mFrameLock);
            mFrameQueue.clear();
            auto dec = mDataContext->decoderContext;
            avcodec_flush_buffers(dec);
            sendMessage(MESSAGE_ID_PROCESS_PACKAGE);
        } else {
            mDataContext->resendPkt = pkt;
            onMessageResendPkt();
        }
    }

    void VideoRender::onMessageResendPkt() {
        auto pkt = mDataContext->resendPkt;
        auto dec = mDataContext->decoderContext;
        int action = mQueueRef->getTopAction();
        if (action == NEXT_INDEX_SEEK || action == NEXT_INDEX_TRACK_CHANGED) {
            mThread.messageQueue().removeMessageById(MESSAGE_ID_RECEIVE_FRAME);
            mDataContext->resendPkt = nullptr;
            av_packet_free(&pkt);
            avcodec_flush_buffers(dec);
            sendMessage(MESSAGE_ID_PROCESS_PACKAGE);
            return;
        }

        int ret = 0;


//        auto videoFrame = reusedVideoFrame;

//        if (!(pkt->flags & AV_PKT_FLAG_KEY))
//        {
//            mDataContext->resendPkt = nullptr;
//            av_packet_free(&pkt);
//            sendMessage(MESSAGE_ID_PROCESS_PACKAGE);
//            return;
//        }

        ret = avcodec_send_packet(dec, pkt);

        if (ret != AVERROR(EAGAIN) && ret < 0 && ret != AVERROR_EOF) {
            throw DecoderException(ret);
        }

        if (ret == AVERROR(EAGAIN)) {
            mDataContext->resendPkt = pkt;
            sendMessage(MESSAGE_ID_RESEND_PACKAGE);
        } else {
            mDataContext->resendPkt = nullptr;
            av_packet_free(&pkt);
            sendMessage(MESSAGE_ID_PROCESS_PACKAGE);
        }

        onMessageReceiveFrame();
    }

    void VideoRender::onMessageReceiveFrame() {
        {
            std::lock_guard<std::mutex> l(mFrameLock);
            if (mFrameQueue.isFull()) {
                sendMessage(MESSAGE_ID_RECEIVE_FRAME);
                return;
            }
        }

        int ret = 0;
        auto dec = mDataContext->decoderContext;
        auto videoFrame = reusedVideoFrame;

        ret = avcodec_receive_frame(dec, videoFrame);

        if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            //fixme throw
        }

        if (ret == AVERROR_EOF) {
            return;
        }

        if (ret == AVERROR(EAGAIN)) {
            sendMessageDelay(MESSAGE_ID_RECEIVE_FRAME, 10);
            return;
        }
        sendMessage(MESSAGE_ID_RECEIVE_FRAME);

        onFrame(videoFrame, dec->pkt_timebase);

//        av_frame_unref(videoFrame);
    }

    void VideoRender::onMessagePreSeek() {
        mThread.messageQueue().removeMessageById(MESSAGE_ID_RECEIVE_FRAME);
        mThread.messageQueue().removeMessageById(MESSAGE_ID_RESEND_PACKAGE);
        mThread.messageQueue().removeMessageById(MESSAGE_ID_PROCESS_PACKAGE);

        if (mDataContext->resendPkt != nullptr) {
            av_packet_free(&mDataContext->resendPkt);
            mDataContext->resendPkt = nullptr;
        }

        mDataContext->seek = true;
        sendMessageDelay(MESSAGE_ID_PROCESS_PACKAGE, 10);
    }

    void VideoRender::sendMessage(int id) {
        mThread.messageQueue().pushIfNotExists(Message::simpleMessage(id).withCallback(this));
    }

    void VideoRender::sendMessage(int id, int priority) {
        mThread.messageQueue().pushIfNotExists(Message::simpleMessage(id).priority(priority).withCallback(this));
    }

    void VideoRender::sendMessageDelay(int id, int delayMs) {
        mThread.messageQueue().pushIfNotExists(Message::simpleMessage(id).delay(delayMs).withCallback(this));
    }

    void VideoRender::onFrame(AVFrame *frame, AVRational timebase) {
        if (frame->format != AV_PIX_FMT_YUV420P) {
            throw std::bad_cast();
        }
//        next_log_tag("format", "format: %d", frame->format);
        int64_t p = av_frame_get_best_effort_timestamp(frame);

        p = av_rescale_q(p,
                         timebase,
                         AV_TIME_BASE_Q);

        auto du = av_rescale_q(frame->pkt_duration,
                         timebase,
                         AV_TIME_BASE_Q);

//        auto newFrame = convert(frame);
//            auto newFrame = av_frame_clone(frame);
            auto newFrame = frame;
//        reusedVideoFrame = av_frame_alloc();

//        auto cp = av_frame_alloc();
//        av_frame_copy(cp, frame);
        newFrame->pts = p;
        newFrame->pkt_duration = du;
        std::lock_guard<std::mutex> l(mFrameLock);
        if (cachedVideoFrame != nullptr) {
            reusedVideoFrame = cachedVideoFrame;
            cachedVideoFrame = nullptr;
        } else if (cached2VideoFrame != nullptr) {
            reusedVideoFrame = cached2VideoFrame;
            cached2VideoFrame = nullptr;
        }
        else {
            reusedVideoFrame = av_frame_alloc();
        }
        /*
        if (jump % 2 == 1) {
            jump = 0;
            av_frame_free(&newFrame);
        } else {
            jump += 1;
            mFrameQueue.pushFrame(newFrame);
        }
         */

        mFrameQueue.pushFrame(newFrame);
    }

    void VideoRender::setCurrentFrame() {
        std::lock_guard<std::mutex> l(mFrameLock);
        if (!mFrameQueue.isEmpty()) {
            mCurrentFrame = mFrameQueue.pop();
        }
    }

    void VideoRender::releaseAndSetCurrentFrame() {
        std::lock_guard<std::mutex> l(mFrameLock);

        if (mCurrentFrame != nullptr) {
            av_frame_free(&mCurrentFrame);
            mCurrentFrame = nullptr;
        }

        if (!mFrameQueue.isEmpty()) {
            mCurrentFrame = mFrameQueue.pop();
        }

    }


    //gl thread
    void VideoRender::onSurfaceCreated() {
//        auto *textures = new GLuint[1]; //生成纹理id
        glGenTextures(  //创建纹理对象
                1, //产生纹理id的数量
                textures
        );
        texture = textures[0];

//        __android_log_print(6, "AudioConverter", "create texture %d %d", texture, texture1);

        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_MIN_FILTER, GL_NEAREST);//设置MIN 采样方式
        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_MAG_FILTER, GL_LINEAR);//设置MAG采样方式
        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);//设置S轴拉伸方式
        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);//设置T轴拉伸方式


        glGenFramebuffers(1,&fbo);
        nx_effect::checkGlError("glGenFramebuffers");


        glGenTextures(  //创建纹理对象
                1, //产生纹理id的数量
                &target
        );

        glBindTexture(GL_TEXTURE_2D, target);

        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_MIN_FILTER, GL_NEAREST);//设置MIN 采样方式
        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_MAG_FILTER, GL_LINEAR);//设置MAG采样方式
        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);//设置S轴拉伸方式
        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);//设置T轴拉伸方式

    }

    //gl thread
    void VideoRender::prepareFrame() {

        auto first = mFrameQueue.first();
        if (first == nullptr) {
            return;
        }

        if (mCurrentFrame == nullptr) {
            mCurrentFrame = first;
            mFrameQueue.pop();
            return;
        }


        auto clockTime = mClock->getPts();

        if (first->pts > clockTime) {
            return;
        }

        if (cachedVideoFrame == nullptr) {
            cachedVideoFrame = mCurrentFrame;
            mCurrentFrame = nullptr;
        } else if (cached2VideoFrame == nullptr) {
            cached2VideoFrame = mCurrentFrame;
            mCurrentFrame = nullptr;
        }
        else {

            av_frame_free(&mCurrentFrame);
        }

        while (true) {
            mCurrentFrame = first;
            mFrameQueue.pop();

            if (first->pts < clockTime - 100000) {
                first = mFrameQueue.first();
                if (first == nullptr) {
                    return;
                } else {
                    if (cachedVideoFrame == nullptr) {
                        cachedVideoFrame = mCurrentFrame;
                        mCurrentFrame = nullptr;
                    } else if (cached2VideoFrame == nullptr) {
                        cached2VideoFrame = mCurrentFrame;
                        mCurrentFrame = nullptr;
                    }
                    else {

                        av_frame_free(&mCurrentFrame);
                    }
                }
            } else {
                break;
            }
        }
    }



    //gl thread
    void VideoRender::onDrawFrame() {
        {
            std::lock_guard<std::mutex> l(mFrameLock);

            if (mEffectIndex != mNewEffectIndex) {
                if (effect != nullptr) {
                    delete effect;
                    effect = nullptr;
                }
                mEffectIndex = mNewEffectIndex;
            }

            prepareFrame();
        }
        if (mCurrentFrame == nullptr) {
            return;
        }

        auto rotation = (mBaseRotation + mRotation) % 360;


        if (effect == nullptr) {
            effect = createEffect(mEffectIndex, rotation);

        }

        if (directDraw == nullptr) {
            directDraw = new nx_effect::DirectDraw();
            directDraw->init();
        }

        auto originalFrameWidth = mCurrentFrame->width;
        auto originalFrameHeight = mCurrentFrame->height;

        auto frame = mCurrentFrame;

        if (yuvHelper == nullptr) {
            yuvHelper = new nx_effect::YUVHelper();
        }
        yuvHelper->process_frame(frame);
        auto rgbaTexture = yuvHelper->getRgbaTexture();



        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        nx_effect::checkGlError("glBindFramebuffer");
//        GLint oldFBO;
//        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFBO);


//        GLuint target; //生成纹理id
//        glGenTextures(  //创建纹理对象
//                1, //产生纹理id的数量
//                &target
//        );

//        glTexParameterf(GL_TEXTURE_2D,
//                        GL_TEXTURE_MIN_FILTER, GL_NEAREST);//设置MIN 采样方式
//        glTexParameterf(GL_TEXTURE_2D,
//                        GL_TEXTURE_MAG_FILTER, GL_LINEAR);//设置MAG采样方式
//        glTexParameterf(GL_TEXTURE_2D,
//                        GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);//设置S轴拉伸方式
//        glTexParameterf(GL_TEXTURE_2D,
//                        GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);//设置T轴拉伸方式

        if (!fboProcessed) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, target);
            fboProcessed = true;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, originalFrameWidth, originalFrameHeight, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glBindTexture(GL_TEXTURE_2D, 0);


            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, target, 0);
        }
        nx_effect::checkGlError("glFramebufferTexture2D");

//        unsigned int rbo;
//        glGenRenderbuffers(1, &rbo);
//        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
//        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, originalFrameWidth, originalFrameHeight);
//        glBindRenderbuffer(GL_RENDERBUFFER, 0);
//        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            throw std::bad_exception();
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rgbaTexture);
//        glBindTexture(GL_TEXTURE_2D, texture);
//        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mCurrentFrame->width, mCurrentFrame->height, 0,
//                     GL_RGBA, GL_UNSIGNED_BYTE, mCurrentFrame->data[0]);
//        glBindTexture(GL_TEXTURE_2D, 0);


        glViewport(0, 0, originalFrameWidth, originalFrameHeight);
        effect->draw(0, rgbaTexture, mCurrentFrame->width, mCurrentFrame->height, 0);
#if 0
        GLubyte* pixels = new GLubyte[originalFrameWidth * originalFrameHeight * 4];
        glReadPixels(0,0, originalFrameHeight, originalFrameHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
//        lodepng_encode32_file((std::string("/sdcard/Download/tmp/image-") + std::to_string(nowMicro()) + ".png").c_str(), (const unsigned char *)pixels, originalFrameWidth, originalFrameHeight);
        lodepng_encode32_file((std::string("/sdcard/Download/tmp/image-") + std::to_string(1) + ".png").c_str(), (const unsigned char *)pixels, originalFrameWidth, originalFrameHeight);

        delete pixels;
#endif
//        glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);

        auto texturePadding = mCurrentFrame->channels;

        //calculate texture array
        auto tx0 = texturePadding / (float)(originalFrameWidth);
        auto ty0 = 0.0f;
        auto tx1 = (originalFrameWidth - texturePadding) / (float)(originalFrameWidth);
        auto ty1 = 0.0f;
        auto tx2 = tx1;
        auto ty2 = 1.0f;
        auto tx3 = tx0;
        auto ty3 = 1.0f;

        GLfloat textureArray[] = {tx0, ty0, tx1, ty1, tx2, ty2, tx3, ty3};
        if (rotation == 90) {
            textureArray[0] = tx1;
            textureArray[1] = ty1;

            textureArray[2] = tx2;
            textureArray[3] = ty2;

            textureArray[4] = tx3;
            textureArray[5] = ty3;

            textureArray[6] = tx0;
            textureArray[7] = ty0;
        } else if (rotation == 180) {
            textureArray[0] = tx2;
            textureArray[1] = ty2;

            textureArray[2] = tx3;
            textureArray[3] = ty3;

            textureArray[4] = tx0;
            textureArray[5] = ty0;

            textureArray[6] = tx1;
            textureArray[7] = ty1;
        } else if (rotation == 270) {
            textureArray[0] = tx3;
            textureArray[1] = ty3;

            textureArray[2] = tx0;
            textureArray[3] = ty0;

            textureArray[4] = tx1;
            textureArray[5] = ty1;

            textureArray[6] = tx2;
            textureArray[7] = ty2;
        }
        //calculate vertex array
        auto realFrameWith = mCurrentFrame->width - texturePadding * 2;
        auto realFrameHeight = mCurrentFrame->height;

        if (rotation == 90 || rotation == 270) {
            auto tmp = realFrameWith;
            realFrameWith = realFrameHeight;
            realFrameHeight = tmp;
        }

        auto scaleW = width /(float ) realFrameWith;
        auto scaleH = height /(float) realFrameHeight;
        auto scale = std::min(scaleW, scaleH);
        auto scaleWidth = realFrameWith * scale;
        auto scaleHeight = realFrameHeight * scale;

        float rate1 = width / (float )scaleWidth;
        float rate2 = height / (float )scaleHeight;

        if (rate1 > rate2) {
            auto vx0 = -(scaleWidth) /(float ) width;
            auto vy0 = -1.0f;
            auto vx1 = (scaleWidth) /(float ) width;
            auto vy1 = vy0;
            auto vx2 = vx1;
            auto vy2 = 1.0f;
            auto vx3 = vx0;
            auto vy3 = vy2;

            GLfloat vertex[] = {vx0, vy0, vx1, vy1, vx2, vy2, vx3, vy3};

            directDraw->draw(target, vertex, textureArray);
        } else {
            auto vx0 = -1.f;
            auto vy0 = -(scaleHeight)/(float )height;
            auto vx1 = 1.0f;
            auto vy1 = vy0;
            auto vx2 = vx1;
            auto vy2 = (scaleHeight)/(float )height;
            auto vx3 = vx0;
            auto vy3 = vy2;

            GLfloat vertex[] = {vx0, vy0, vx1, vy1, vx2, vy2, vx3, vy3};

            directDraw->draw(target, vertex, textureArray);
        }




        /*
        //calculate texture array
        auto texturePadding = mCurrentFrame->channels;
        auto tx0 = texturePadding / (float)(originalFrameWidth);
        auto tx1 = (originalFrameWidth - texturePadding) / (float)(originalFrameWidth);
        auto tx2 = tx1;
        auto tx3 = tx0;

        //calculate vertex array
        auto realFrameWith = mCurrentFrame->width - texturePadding * 2;
        auto realFrameHeight = mCurrentFrame->height;

        */

//        glDeleteFramebuffers(1, &fbo);

//        glDeleteTextures(1, &target);
//        glDeleteRenderbuffers(1, &rbo);

        /*
        auto paddingRight = mCurrentFrame->channels;
        int frameWidth = mCurrentFrame->width - paddingRight * 2;
        int frameHeight = mCurrentFrame->height;
//        auto rate = calculateRate(frameWidth, frameHeight, width, height);
        float rate1 = width / (float )frameWidth;
        float rate2 = height / (float )frameHeight;

        if (rate1 > rate2) {
            int width1 = frameWidth * rate2;
            auto x = (width - width1) / 2;
            glViewport(x, 0, width1, height);
        } else {
            int height1 = frameHeight * rate1;
            auto y = (height - height1) / 2;
            glViewport(0, 0, width, height1);
        }
//        next_log_tag("draw", "%d %d, %d %d", frameWidth, frameHeight, width, height);
//        glViewport(0, 0, frameWidth * rate, frameHeight*rate);
//        glViewport(0, 0, width, height);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mCurrentFrame->width, mCurrentFrame->height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, mCurrentFrame->data[0]);
//        queueFree.push(pFrame);
//        glBindTexture(GL_TEXTURE_2D, 0);

        effect->draw(0, texture, mCurrentFrame->width, mCurrentFrame->height, paddingRight);
         */
    }

    //gl thread
    void VideoRender::onSurfaceChanged(int w, int h) {
        glViewport(0, 0, w, h/2);
        width = w;
        height = h;
    }

    void VideoRender::stop() {
        mStopped.store(true);
        mThread.join();
    }

    bool VideoRender::isStopped() {
        return mStopped.load();
    }

    void VideoRender::updateEffect(int effectIndex) {
        std::lock_guard<std::mutex> l(mFrameLock);

        if (effectIndex == mEffectIndex) {
            return;
        }

        mNewEffectIndex = effectIndex;
//        mEffectIndex = effectIndex;

//        if (effect != nullptr) {
//            delete effect;
//            effect = nullptr;
//        }

    }

    void VideoRender::rotate(int rotation) {
        std::lock_guard<std::mutex> l(mFrameLock);
        mRotation = rotation;
    }

    void VideoRender::preSeek() {
        sendMessage(MESSAGE_ID_PRE_SEEK, MESSAGE_PRIORITY_PRE_SEEK);
    }
}
