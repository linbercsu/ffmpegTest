//
// Created by linlin zhao on 2024/5/23.
//

#include "Reader.h"
#include <thread>
#include <chrono>
#include <android/log.h>

extern "C" {
    #include "libavformat/avformat.h"
}

#define next_log(format, ...) __android_log_print(6, "reader", format, __VA_ARGS__)

namespace next {
    class ContextData {
    public:
        AVFormatContext *fmt_ctx = nullptr;
        AVCodecContext *audio_dec_ctx = nullptr;
        AVCodecContext *video_dec_ctx = nullptr;
//        int64_t audioDurationAdded = 0;
//        int64_t videoDurationAdded = 0;
        int width = 0, height = 0;
        enum AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;
        AVStream *audio_stream = nullptr;
        AVStream *video_stream = nullptr;
        const char *src_filename = nullptr;
        int audio_stream_idx = -1;
        int video_stream_idx = -1;
        AVFrame *mAudioFrame = nullptr;
        AVFrame *videoFrame = nullptr;
        AVPacket *pkt = nullptr;
    };


    Reader::Reader(std::string path, VideoPackageQueue* videoPackageQueue, VideoPackageQueue* audioPackageQueue):mPath(path), mVideoPktQueueRef(videoPackageQueue), mAudioPktQueueRef(audioPackageQueue) {
        mContextData = new ContextData();
        mThread = new std::thread(&Reader::run, this);
    }

    void Reader::run() {
        open();
    }

    void Reader::open() {
        int ret = 0;
        ret = avformat_open_input(&(mContextData->fmt_ctx), mPath.c_str(), nullptr, nullptr);
        next_log("avformat_open_input ret %d, %d", ret, __LINE__);
        auto fmt_ctx = mContextData->fmt_ctx;

        ret = avformat_find_stream_info(fmt_ctx, nullptr);
        next_log("avformat_find_stream_info ret %d, %d", ret, __LINE__);

        ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        next_log("av_find_best_stream ret %d, %d", ret, __LINE__);

        auto audio_stream_index = ret;
        auto audio_stream = fmt_ctx->streams[audio_stream_index];
        mAudioPktQueueRef->onCodecParametersGot(audio_stream->codecpar, audio_stream->time_base);


        ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        next_log("av_find_best_stream ret %d, %d", ret, __LINE__);

        auto video_stream_index = ret;
        auto video_stream = fmt_ctx->streams[video_stream_index];

        mVideoPktQueueRef->onCodecParametersGot(video_stream->codecpar, video_stream->time_base);

        auto pktCount = 0;
        while (true) {
            AVPacket * pkt = av_packet_alloc();
            ret = av_read_frame(fmt_ctx, pkt);
            if (ret < 0) {
                av_packet_free(&pkt);
                next_log("av_read_frame ret %d, %s, %d, %d", ret, av_err2str(ret), pktCount, __LINE__);
                break;
            }

            pktCount++;
            if (pkt->stream_index == audio_stream_index) {
                while (true) {
                    if (mAudioPktQueueRef->enqueue(pkt)) {
                        break;
                    }

                    next_log("enqueue full %d, %d", pktCount, __LINE__);

                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            } else if (pkt->stream_index == video_stream_index) {
                while (true) {
                    if (mVideoPktQueueRef->enqueue(pkt)) {
                        break;
                    }

//                    next_log("enqueue full %d, %d", pktCount, __LINE__);

                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        }

        mVideoPktQueueRef->end();
    }

    void Reader::start() {

    }

    void Reader::pause() {

    }

    void Reader::stop() {

    }

    void Reader::seek() {

    }
}