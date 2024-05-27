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
#include "libyuv.h"
#include "ZEffect.h"
#include "GrayEffect.h"
#include "MirrorEffect.h"
#include "Releasable.h"

extern "C" {
#include "libavformat/avformat.h"
}

namespace next {
    bool lastRender = false;

    namespace {
        nx_effect::BaseEffect* createEffect(int effectIndex) {
            nx_effect::BaseEffect *effect;
            if (effectIndex == 1) {
                effect = new nx_effect::MirrorEffect();
            } else if (effectIndex == 2) {
                effect = new nx_effect::GrayEffect();
            } else {
                effect = new nx_effect::ZEffect();
            }

            effect->init();

            return effect;
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
        auto videoFrameConvertRGBA = alloc_picture(AV_PIX_FMT_RGBA, pFrame->width,
                                                   pFrame->height);

        av_frame_make_writable(videoFrameConvertRGBA);


        libyuv::I420ToABGR(pFrame->data[0], pFrame->linesize[0],
                           pFrame->data[1], pFrame->linesize[1],
                           pFrame->data[2], pFrame->linesize[2],

                           videoFrameConvertRGBA->data[0], videoFrameConvertRGBA->linesize[0],
                           pFrame->width,
                           pFrame->height

        );

        return videoFrameConvertRGBA;
    }


    VideoRender::VideoRender(next::VideoPackageQueue *pQueue, MediaClock* clock) : mQueueRef(pQueue),
                                                                mFrameQueue(1024 * 1024 * 250), mClock(clock) {
        mThread = new std::thread(&VideoRender::run, this);
//        effect = createEffect(mEffectIndex);
    }

    VideoRender::~VideoRender() {
        next_log_tag("video", "delete VideoRender %d", __LINE__);
        delete mThread;
        mThread = nullptr;

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

        delete mDataContext;
        mDataContext = nullptr;

        glDeleteTextures(1, textures);
    }

    void VideoRender::run() {
        Releasable<VideoRender> r(this);
        mDataContext = new DataContext();
        runInternal();

    }

    void VideoRender::runInternal() {
        int ret = 0;
        AVCodecParameters *codecParameters = nullptr;
        AVRational timeBase;

        while (!isStopped()) {
            codecParameters = mQueueRef->getCodecParameters();

            if (codecParameters == nullptr) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            timeBase = mQueueRef->getTimebase();

            break;
        }

        if (isStopped()) {
            return;
        }

        auto decoder = avcodec_find_decoder(codecParameters->codec_id);

        mDataContext->decoderContext = avcodec_alloc_context3(decoder);
        auto dec_ctx = mDataContext->decoderContext;

        ret = avcodec_parameters_to_context(dec_ctx, codecParameters);
        dec_ctx->pkt_timebase = timeBase;

        AVDictionary *opts = nullptr;
        ret = avcodec_open2(dec_ctx, decoder, &opts);
        mDataContext->decoder = decoder;

        reusedVideoFrame = av_frame_alloc();
        while (!isStopped()) {
            bool clear = false;
            AVPacket *pkt = mQueueRef->getPkt(&clear);
            if (clear) {
                mFrameQueue.clear();
                avcodec_flush_buffers(dec_ctx);
            }

//            AVPacket *pkt = mQueueRef->getPkt(nullptr);

            if (pkt == nullptr) {
                render();
//                next_log("no video pkt detected. %d", __LINE__);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }

            decode(dec_ctx, pkt, reusedVideoFrame);

//            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            av_packet_free(&pkt);

            render();
        }
    }

    void VideoRender::onFrame(AVFrame *frame, AVRational timebase) {
//        next_log_tag("format", "format: %d", frame->format);
        int64_t p = av_frame_get_best_effort_timestamp(frame);

        p = av_rescale_q(p,
                         timebase,
                         AV_TIME_BASE_Q);

        auto newFrame = convert(frame);

//        auto cp = av_frame_alloc();
//        av_frame_copy(cp, frame);
        newFrame->pts = p;
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

    void VideoRender::render() {
        if (mFrameQueue.isEmpty()) {
            next_log_tag("render", "no video frame %d", __LINE__);
            if (mQueueRef->isEnd()) {
                if (!lastRender) {
                    lastRender = true;
                    next_log_tag("render", "last frame %d", __LINE__);
                }
            }
            return;
        }

        if (mCurrentFrame == nullptr) {
            setCurrentFrame();
            //render first frame

//            mClock.display();
            next_log_tag("render", "first frame %ld, %d", mCurrentFrame->pts, __LINE__);
            return;
        }

        int64_t lastPts = 0;
        bool lastPtsSet = false;
        while (!isStopped()) {
            auto next = mFrameQueue.first();
//            auto duration = next->pts - mCurrentFrame->pts;
            int64_t pts = mClock->getPts();
            if (!lastPtsSet) {
                lastPtsSet = true;
                lastPts = pts;
            }

            //backward
            if (lastPts > pts) {
                next_log("seek backward detected. %d", __LINE__);
                break;
            }

//            //seek backward
//            if (mCurrentFrame->pts >= next->pts) {
//                next_log("seek backward detected. %d", __LINE__);
//                releaseAndSetCurrentFrame();
//                break;
//            }

            if (pts >= next->pts) {
                //next frame
                releaseAndSetCurrentFrame();
            } else {
                if (mFrameQueue.isFull()) {
//                    next_log_tag("video", "frame queue full, and dispaly current frame %ld, %d", mCurrentFrame->pts, __LINE__);
                    std::this_thread::sleep_for(std::chrono::milliseconds (10));
                    continue;
                }
//                next_log_tag("video", "render exit %d", __LINE__);
                break;
            }

            if (mFrameQueue.isEmpty()) {
                break;
            }

        }
    }

    void VideoRender::decode(AVCodecContext *dec, const AVPacket *package, AVFrame *videoFrame) {

        int ret;
        ret = avcodec_send_packet(dec, package);

//        __android_log_print(6, "MediaConverter", "on video pkt %ld %d", package->pts, ret);
        if (ret != AVERROR(EAGAIN) && ret < 0) {
            throw DecoderException(ret);
        }


        while (ret == AVERROR(EAGAIN)) {
            next_log_tag("video", "again error detected, %d", __LINE__);
            while (!isStopped()) {
                ret = avcodec_receive_frame(dec, videoFrame);
                if (ret < 0) {
                    if (ret == AVERROR_EOF)
                        return;

                    if (ret == AVERROR(EAGAIN)) {
                        ret = avcodec_send_packet(dec, package);
                        if (ret == 0)
                            break;

                        if (ret < 0 && ret != AVERROR(EAGAIN)) {
                            throw DecoderException(ret);
                        }

                        next_log_tag("video", "send pkt failed, %d", __LINE__);

                        continue;
                    }

                    throw DecoderException(ret);
                }

                onFrame(videoFrame, dec->pkt_timebase);

                av_frame_unref(videoFrame);
                ret = avcodec_send_packet(dec, package);
                break;
            }
        }

        if (ret < 0) {
            throw DecoderException(ret);
        }


        // get all the available frames from the decoder
        while (!isStopped()) {
            ret = avcodec_receive_frame(dec, videoFrame);
            if (ret < 0) {
                // those two return values are special and mean there is no output
                // frame available, but there were no errors during decoding
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                    return;

                throw DecoderException(ret);
            }

            onFrame(videoFrame, dec->pkt_timebase);

            av_frame_unref(videoFrame);
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


    }

    //gl thread
    void VideoRender::onDrawFrame() {
        std::lock_guard<std::mutex> l(mFrameLock);
        if (mCurrentFrame == nullptr) {
            return;
        }

        if (effect == nullptr) {
            effect = createEffect(mEffectIndex);
        }

        int frameWidth = mCurrentFrame->width;
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
            glViewport(0, y, width, height1);
        }
//        next_log_tag("draw", "%d %d, %d %d", frameWidth, frameHeight, width, height);
//        glViewport(0, 0, frameWidth * rate, frameHeight*rate);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mCurrentFrame->width, mCurrentFrame->height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, mCurrentFrame->data[0]);
//        queueFree.push(pFrame);
//        glBindTexture(GL_TEXTURE_2D, 0);

        effect->draw(0, texture, width, height);
    }

    //gl thread
    void VideoRender::onSurfaceChanged(int w, int h) {
        glViewport(0, 0, w, h/2);
        width = w;
        height = h;
    }

    void VideoRender::stop() {
        mStopped.store(true);
        mThread->join();
    }

    bool VideoRender::isStopped() {
        return mStopped.load();
    }

    void VideoRender::updateEffect(int effectIndex) {
        if (effectIndex == mEffectIndex) {
            return;
        }

        mEffectIndex = effectIndex;
        std::lock_guard<std::mutex> l(mFrameLock);
        if (effect != nullptr) {
            delete effect;
            effect = nullptr;
        }

//        effect = createEffect(effectIndex);
    }
}
