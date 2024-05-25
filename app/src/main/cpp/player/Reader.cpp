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
        ~ContextData() {
            if (fmt_ctx != nullptr) {
                avformat_close_input(&fmt_ctx);
                fmt_ctx = nullptr;
            }
        }

        AVFormatContext *fmt_ctx = nullptr;
//        AVCodecContext *audio_dec_ctx = nullptr;
//        AVCodecContext *video_dec_ctx = nullptr;
//        int64_t audioDurationAdded = 0;
//        int64_t videoDurationAdded = 0;
//        int width = 0, height = 0;
//        enum AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;
//        AVStream *audio_stream = nullptr;
//        AVStream *video_stream = nullptr;
//        const char *src_filename = nullptr;
//        int audio_stream_idx = -1;
//        int video_stream_idx = -1;
//        AVFrame *mAudioFrame = nullptr;
//        AVFrame *videoFrame = nullptr;
//        AVPacket *pkt = nullptr;
    };


    Reader::Reader(std::string path, VideoPackageQueue* videoPackageQueue, VideoPackageQueue* audioPackageQueue, ReaderCallback* callback):mPath(path), mVideoPktQueueRef(videoPackageQueue), mAudioPktQueueRef(audioPackageQueue), mReaderCallback(callback) {
        mContextData = new ContextData();
        mThread = new std::thread(&Reader::run, this);
    }

    Reader::~Reader() {
        delete mThread;
        mThread = nullptr;

    }

    void Reader::run() {
        open();

        if (mContextData != nullptr) {
            delete mContextData;
            mContextData = nullptr;
        }

        mVideoPktQueueRef->clear();
        mAudioPktQueueRef->clear();
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

        auto audioDuration = av_rescale_q(audio_stream->duration,
                                     audio_stream->time_base,
                                     AV_TIME_BASE_Q);

        mReaderCallback->onDurationKnown(audioDuration);
        AVPacket * pkt = nullptr;
        auto pktCount = 0;
        while (!isStopped()) {
//            if (pkt != nullptr) {
//                av_packet_free(&pkt);
//                pkt = nullptr;
//            }

            int64_t seek = getSeekPosition();
            if (seek != -1) {
                int64_t start = av_rescale_q(seek,
                                                  AV_TIME_BASE_Q,
                                                  audio_stream->time_base);
                ret = av_seek_frame(fmt_ctx, audio_stream_index, start, AVSEEK_FLAG_BACKWARD);
                if (ret != 0) {
                    throw std::bad_cast();
                }
                start = av_rescale_q(seek,
                                                  AV_TIME_BASE_Q,
                                                  video_stream->time_base);
                ret = av_seek_frame(fmt_ctx, video_stream_index, start, AVSEEK_FLAG_BACKWARD);
                if (ret != 0) {
                    throw std::bad_cast();
                }
                mVideoPktQueueRef->setNeedClear();
                mAudioPktQueueRef->setNeedClear();

                next_log("process seek %ld, %d", seek, __LINE__);
            }

            if (pkt == nullptr) {
                pkt = av_packet_alloc();
            } else {
                av_packet_unref(pkt);
            }

            ret = av_read_frame(fmt_ctx, pkt);
            if (ret < 0) {
//                av_packet_free(&pkt);
//                pkt = nullptr;
                if (ret == AVERROR_EOF) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue;
                } else {
                    next_log("av_read_frame ret %d, %s, %d, %d", ret, av_err2str(ret), pktCount, __LINE__);
                    break;
                }
            }

            pktCount++;
            if (pkt->stream_index == audio_stream_index) {
                while (!isStopped()) {
                    if (mAudioPktQueueRef->enqueue(pkt)) {
                        pkt = nullptr;
                        break;
                    }

                    next_log("enqueue full %d, %d", pktCount, __LINE__);

                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            } else if (pkt->stream_index == video_stream_index) {
                while (!isStopped()) {
                    if (mVideoPktQueueRef->enqueue(pkt)) {
                        pkt = nullptr;
                        break;
                    }

//                    next_log("enqueue full %d, %d", pktCount, __LINE__);

                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        }

        if (pkt != nullptr) {
            av_packet_free(&pkt);
            pkt = nullptr;
        }

        mVideoPktQueueRef->end();

        while (!isStopped()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            //todo
        }
    }

    void Reader::stop() {
        stopped.store(true);
        mThread->join();
    }

    void Reader::start() {

    }

    void Reader::pause() {

    }

    void Reader::seek() {

    }

    bool Reader::isStopped() {
        return stopped.load();
    }

    void Reader::seekBackward(int64_t newPosition) {
        next_log("seekBackward %ld, %d", newPosition, __LINE__);
        auto seek = newPosition | 0x8000000000000000L;
        mSeekPosition.store(seek);
    }

    void Reader::seekForward(int64_t newPosition) {
        next_log("seekForward %ld, %d", newPosition, __LINE__);
        auto seek = newPosition | 0x8000000000000000L;
        mSeekPosition.store(seek);
    }

    int64_t Reader::getSeekPosition() {
        int64_t seek = mSeekPosition.load();
        if ((seek & 0x8000000000000000L) == 0) {
            return -1;
        } else {
            mSeekPosition.store(0);
            return seek & 0x7fffffffffffffff;    
        }
    }
}