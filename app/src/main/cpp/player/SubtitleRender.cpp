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

    SubtitleRender::SubtitleRender(next::VideoPackageQueue *pQueue, MediaClock *clock): mQueueRef(pQueue), mMediaClockRef(clock), mThread(this) {
        sendMessage(MESSAGE_ID_INIT, MESSAGE_PRIORITY_INIT);
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
            default:
                break;
        }

    }

    void SubtitleRender::onThreadEnded() {
        if (mDataContext != nullptr) {
            delete mDataContext;
            mDataContext = nullptr;
        }
    }

    void SubtitleRender::onMessageInit() {
        int ret = 0;

        auto codecParameters = mQueueRef->getCodecParameters();

        if (codecParameters == nullptr) {
            sendMessageDelay(MESSAGE_ID_INIT, MESSAGE_PRIORITY_INIT);
            return;
        }

        mDataContext = new DataContext();
        AVRational timeBase = mQueueRef->getTimebase();
        mDataContext->timebase = timeBase;

//        mAudioDevice->withSourceCodecParameter(codecParameters);
//        mAudioDevice->open();

        auto decoder = avcodec_find_decoder(codecParameters->codec_id);

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
        auto clock = mMediaClockRef->getPts();
        auto pkt = mQueueRef->getPkt(&clear);

        if (clear) {

        }

        if (pkt != nullptr && pkt->stream_index == -1) {
            av_packet_free(&pkt);
            pkt = nullptr;
        }

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
//        if (new_pts  <= clock && new_pts + duration >= clock)
        {
            int got_sub = 0;
            AVSubtitle subtitle;
            int ret = avcodec_decode_subtitle2(dec, &subtitle,
                                               &got_sub, pkt);

            if (got_sub) {

                next_log("subtitle onMessageRender begin==============, %d" , __LINE__);
                for (int i = 0; i < subtitle.num_rects; i++) {
                    next_log("subtitle onMessageRender text-> %s,  %d", subtitle.rects[i]->text, __LINE__);
                    next_log("subtitle onMessageRender ass-> %s,  %d", subtitle.rects[i]->ass, __LINE__);
                }
                next_log("subtitle onMessageRender end================, %d" , __LINE__);
                avsubtitle_free(&subtitle);
            }
        }

        av_packet_free(&pkt);
        sendMessageDelay(MESSAGE_ID_RENDER, 100);
    }

    void SubtitleRender::sendMessage(int id) {
        mThread.messageQueue().pushBack(Message::simpleMessage(id).withCallback(this));
    }

    void SubtitleRender::sendMessage(int id, int priority) {
        mThread.messageQueue().pushBack(Message::simpleMessage(id).priority(priority).withCallback(this));
    }

    void SubtitleRender::sendMessageDelay(int id, int delayMs) {
        mThread.messageQueue().pushBack(Message::simpleMessage(id).delay(delayMs).withCallback(this));
    }

}
