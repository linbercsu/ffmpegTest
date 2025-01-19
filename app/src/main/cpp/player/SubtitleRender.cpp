//
// Created by Zhao, Linlin on 11/1/25.
//

#include "SubtitleRender.h"
#include "VideoPackageQueue.h"
#include "Log.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
}


namespace next {
    namespace {
        const int MESSAGE_ID_INIT = Message::MESSAGE_ID_USER + 1;
        const int MESSAGE_ID_RENDER = MESSAGE_ID_INIT + 1;
        const int MESSAGE_ID_RENDER_DELAY = MESSAGE_ID_RENDER + 1;

        const int MESSAGE_PRIORITY_INIT = Message::MESSAGE_PRIORITY_NORMAL + 2;
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
        AVRational timebase;
        bool seek{false};
    };

    void SubtitleRender::stop() {

    }

    SubtitleRender::SubtitleRender(JNIEnv *env, jobject javaPlayer, next::VideoPackageQueue *pQueue, MediaClock *clock):mQueueRef(pQueue), mMediaClockRef(clock), mThread(this) {
        env->GetJavaVM(&jvm);
        javaPlayerRef = env->NewGlobalRef(javaPlayer);
        sendMessage(MESSAGE_ID_INIT, MESSAGE_PRIORITY_INIT);
        jclass cl = env->GetObjectClass(javaPlayer);
        methodId = env->GetMethodID(cl, "displaySubtitle", "(Ljava/lang/String;)V");
    }

    void SubtitleRender::handleMessage(const Message &message) {
        switch (message.getId()) {
            case MESSAGE_ID_INIT: {
                onMessageInit();
                break;
            }
            case MESSAGE_ID_RENDER: {
                onMessageRender();
                break;
            }

            case MESSAGE_ID_RENDER_DELAY: {
                onMessageRenderDelay();
                break;
            }
            default:
                break;
        }

    }


    void SubtitleRender::onThreadStarted() {
        JNIEnv *env = nullptr;

        int err = jvm->GetEnv( (void**)&env, JNI_VERSION_1_2 );
        if( err != JNI_OK )
        {
            if( err != JNI_EDETACHED )
            {
                next_log("SubtitleRender::onThreadStarted() GetEnv() error=%d %d", err, __LINE__);
                if( err == JNI_EVERSION ) {

                }
            }

            err = jvm->AttachCurrentThread( &env, 0 );
//            DBG_v( TAG, "Java VM attached to thread #{0}. error={1}", gettid(), err );
            if( err != JNI_OK )
            {
//                LOG_e( TAG, "JavaVM::GetEnv() failed. error={0}", err );
//                throw JNIError( err );
            }

        }

        jvmEnv = env;
    }

    void SubtitleRender::onThreadEnded() {
        if (mDataContext != nullptr) {
            delete mDataContext;
            mDataContext = nullptr;
        }

        jvmEnv->DeleteGlobalRef(javaPlayerRef);
        jvm->DetachCurrentThread();
    }

    void SubtitleRender::onMessageInit() {
//        next_log("SubtitleRender::onMessageInit() %d", __LINE__);
        int ret = 0;

        auto codecParameters = mQueueRef->getCodecParameters();

        if (codecParameters == nullptr) {
            sendMessageDelay(MESSAGE_ID_INIT, MESSAGE_PRIORITY_INIT);
            return;
        }

        if (mDataContext != nullptr) {
            delete mDataContext;
            mDataContext = nullptr;
        }

        mDataContext = new DataContext();
        AVRational timeBase = mQueueRef->getTimebase();
        mDataContext->timebase = timeBase;

//        mAudioDevice->withSourceCodecParameter(codecParameters);
//        mAudioDevice->open();

        auto decoder = avcodec_find_decoder(codecParameters->codec_id);
        next_log("SubtitleRender::onMessageInit() %d %d, %d", codecParameters->codec_id, AV_CODEC_ID_WEBVTT, __LINE__);
        auto dec_ctx = avcodec_alloc_context3(decoder);
        mDataContext->decoderContext = dec_ctx;
        ret = avcodec_parameters_to_context(dec_ctx, codecParameters);
        dec_ctx->pkt_timebase = timeBase;

        AVDictionary *opts = nullptr;
        ret = avcodec_open2(dec_ctx, decoder, &opts);

        sendMessageDelay(MESSAGE_ID_RENDER, 100);

    }


    void SubtitleRender::onMessageRender() {
        bool clear = false;
        auto pkt = mQueueRef->getPkt(&clear);
        if (pkt == nullptr) {
            sendMessageDelay(MESSAGE_ID_RENDER, 100);
            return;
        }

        if (pkt->stream_index == NEXT_INDEX_TRACK_CHANGED) {
            av_packet_free(&pkt);
            sendMessage(MESSAGE_ID_INIT, MESSAGE_PRIORITY_INIT);
            return;
        } else if (pkt->stream_index == NEXT_INDEX_END) {
            av_packet_free(&pkt);
            pkt = nullptr;
            sendMessageDelay(MESSAGE_ID_RENDER, 100);
            return;
        } else if (pkt->stream_index == NEXT_INDEX_SEEK) {
            mDataContext->seek = false;
            auto dec = mDataContext->decoderContext;
            avcodec_flush_buffers(dec);
            av_packet_free(&package);
            sendMessageDelay(MESSAGE_ID_RENDER, 100);
            return;
        }

        package = pkt;
        onMessageRenderDelay();
    }

    void SubtitleRender::onMessageRenderDelay() {
        auto clock = mMediaClockRef->getPts();
        auto pkt = package;
        package = nullptr;
        if (pkt == nullptr) {
            sendMessageDelay(MESSAGE_ID_RENDER, 100);
            return;
        }

        auto dec = mDataContext->decoderContext;

        auto new_pts = av_rescale_q(pkt->pts,
                                    mDataContext->timebase,
                                    AV_TIME_BASE_Q);

        auto duration = av_rescale_q(pkt->duration,
                                     mDataContext->timebase,
                                     AV_TIME_BASE_Q);

        if (new_pts > clock) {
            package = pkt;
            sendMessageDelay(MESSAGE_ID_RENDER_DELAY, 100);
            return;
        }
        next_log("onMessageRender %lld, %lld, %lld, %lld %d", clock, pkt->pts, pkt->duration, new_pts, __LINE__);
        if (new_pts  <= clock && new_pts + duration >= clock)
        {
            int got_sub = 0;
            AVSubtitle subtitle;
            int ret = avcodec_decode_subtitle2(dec, &subtitle,
                                               &got_sub, pkt);

            if (got_sub) {

//                next_log("subtitle onMessageRender begin==============, %d" , __LINE__);
//                for (int i = 0; i < subtitle.num_rects; i++) {
//                    next_log("subtitle onMessageRender text-> %s,  %d", subtitle.rects[i]->text, __LINE__);
//                    next_log("subtitle onMessageRender ass-> %s,  %d", subtitle.rects[i]->ass, __LINE__);
//                }
//                next_log("subtitle onMessageRender end================, %d" , __LINE__);

                displaySubtitle(&subtitle);

                avsubtitle_free(&subtitle);
            }
        }

        av_packet_free(&pkt);
        sendMessageDelay(MESSAGE_ID_RENDER, 100);
    }

    void SubtitleRender::displaySubtitle(struct AVSubtitle* subtitle) {
        char* ass = nullptr;
        if (subtitle->num_rects > 0) {
            ass = subtitle->rects[0]->ass;
        }
        
        if (ass == nullptr) {
            return;
        }

        auto str = jvmEnv->NewStringUTF(ass);

        jvmEnv->CallVoidMethod(javaPlayerRef, methodId, str);
        jvmEnv->ExceptionCheck();

        jvmEnv->DeleteLocalRef(str);
    }

    void SubtitleRender::sendMessage(int id) {
        mThread.messageQueue().pushIfNotExists(Message::simpleMessage(id).withCallback(this));
    }

    void SubtitleRender::sendMessage(int id, int priority) {
        mThread.messageQueue().pushIfNotExists(Message::simpleMessage(id).priority(priority).withCallback(this));
    }

    void SubtitleRender::sendMessageDelay(int id, int delayMs) {
        mThread.messageQueue().pushIfNotExists(Message::simpleMessage(id).delay(delayMs).withCallback(this));
    }

}
