//
// Created by linlin zhao on 2024/5/23.
//

#include "Reader.h"
#include "Releasable.h"
#include <thread>
#include <chrono>
#include <android/log.h>

extern "C" {
    #include "libavformat/avformat.h"
}

#define next_log(format, ...) __android_log_print(6, "reader", format, __VA_ARGS__)

namespace next {
    namespace {
        const int MESSAGE_ID_OPEN = Message::MESSAGE_ID_USER + 1;
        const int MESSAGE_ID_READ_PKG = MESSAGE_ID_OPEN + 1;
        const int MESSAGE_ID_SEEK = MESSAGE_ID_READ_PKG + 1;

        const int MESSAGE_ID_SEND_PKG = MESSAGE_ID_SEEK + 1;



    }

    class ReaderContextData {
    public:
        ~ReaderContextData() {
            if (fmt_ctx != nullptr) {
                avformat_close_input(&fmt_ctx);
                fmt_ctx = nullptr;
            }
        }

        AVFormatContext *fmt_ctx = nullptr;

    };


    Reader::Reader(const std::string& path, VideoPackageQueue* videoPackageQueue, VideoPackageQueue* audioPackageQueue, ReaderCallback* callback):mThread(this), mPath(path), mVideoPktQueueRef(videoPackageQueue), mAudioPktQueueRef(audioPackageQueue), mReaderCallback(callback) {
//        mThread = new std::thread(&Reader::run, this);
        sendMessage(MESSAGE_ID_OPEN);
    }

    Reader::~Reader() {
//        delete mThread;
//        mThread = nullptr;

    }

    void Reader::release() {
        if (mContextData != nullptr) {
            delete mContextData;
            mContextData = nullptr;
        }

        mVideoPktQueueRef->clear();
        mAudioPktQueueRef->clear();
    }

    void Reader::onThreadEnded() {
        release();
    }

    void Reader::handleMessage(const next::Message &message) {
        switch (message.getId()) {
            case MESSAGE_ID_OPEN: {
                onMessageOpen();
                break;
            }

            case MESSAGE_ID_SEEK: {
                onMessageSeek(message.getData2());
                break;
            }

            case MESSAGE_ID_READ_PKG: {
                onMessageReadPackage();
                break;
            }


            case MESSAGE_ID_SEND_PKG: {
                onMessageSendPackage();
                break;
            }

            default:
                break;
        }
    }

    void Reader::run() {
        Releasable<Reader> r(this);
        open();
    }

    void Reader::sendMessage(int id) {
        mThread.messageQueue().pushBack(Message::simpleMessage(id).withCallback(this));
    }

    void Reader::sendMessageDelay(int id, int delayMs) {
        //FIXME
        mThread.messageQueue().pushBack(Message::simpleMessage(id).withCallback(this));
    }

    void Reader::onMessageOpen() {
        mContextData = new ReaderContextData();
        int ret = 0;
        ret = avformat_open_input(&(mContextData->fmt_ctx), mPath.c_str(), nullptr, nullptr);
        next_log("avformat_open_input ret %d, %d", ret, __LINE__);
        auto fmt_ctx = mContextData->fmt_ctx;

        ret = avformat_find_stream_info(fmt_ctx, nullptr);
        next_log("avformat_find_stream_info ret %d, %d", ret, __LINE__);

        ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        next_log("av_find_best_stream video ret %d, %d", ret, __LINE__);

        int mediaDuration = 0;
        video_stream_index = ret;
        if (video_stream_index >= 0) {
            video_stream = fmt_ctx->streams[video_stream_index];
            AVDictionaryEntry *pEntry = av_dict_get(video_stream->metadata, "rotate", nullptr,
                                                    AV_DICT_MATCH_CASE);
            int rotation = 0;
            if (pEntry != nullptr) {
                auto rotationString = pEntry->value;
                if (std::string("90") == rotationString) {
                    rotation = 90;
                } else if (std::string("180") == rotationString) {
                    rotation = 180;
                } else if (std::string("270") == rotationString) {
                    rotation = 270;
                }
            }

            mVideoPktQueueRef->onCodecParametersGot(video_stream->codecpar,
                                                    video_stream->time_base, rotation);


            mediaDuration = av_rescale_q(video_stream->duration,
                                         video_stream->time_base,
                                         AV_TIME_BASE_Q);
            mHasVideo = true;
        }


        ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        next_log("av_find_best_stream audio ret %d, %d", ret, __LINE__);

        audio_stream_index = ret;
        if (audio_stream_index >= 0) {
            audio_stream = fmt_ctx->streams[audio_stream_index];
            mAudioPktQueueRef->onCodecParametersGot(audio_stream->codecpar, audio_stream->time_base,
                                                    0);


            mediaDuration = av_rescale_q(audio_stream->duration,
                                         audio_stream->time_base,
                                         AV_TIME_BASE_Q);

            mHasAudio = true;
        }



        mReaderCallback->onDurationKnown(mediaDuration);

        sendMessage(MESSAGE_ID_READ_PKG);
    }

    void Reader::onMessageSeek(int64_t seek) {
        int ret = 0;
        if (video_stream != nullptr) {
            int64_t start = av_rescale_q(seek,
                                         AV_TIME_BASE_Q,
                                         video_stream->time_base);

            ret = av_seek_frame(mContextData->fmt_ctx, video_stream_index, start, AVSEEK_FLAG_BACKWARD);
            if (ret != 0) {
                throw std::bad_cast();
            }
        } else if (audio_stream != nullptr) {
            int64_t start = av_rescale_q(seek,
                                         AV_TIME_BASE_Q,
                                         audio_stream->time_base);

            ret = av_seek_frame(mContextData->fmt_ctx, audio_stream_index, start, AVSEEK_FLAG_BACKWARD);
            if (ret != 0) {
                throw std::bad_cast();
            }
        }

        mVideoPktQueueRef->clear();
        mAudioPktQueueRef->clear();
        mVideoPktQueueRef->setNeedClear();
        mAudioPktQueueRef->setNeedClear();

        next_log("process seek %ld, %d", seek, __LINE__);
    }
    void Reader::onMessageReadPackage() {
        int ret = 0;
        if (packet == nullptr) {
            packet = av_packet_alloc();
        } else {
            av_packet_unref(packet);
        }

        ret = av_read_frame(mContextData->fmt_ctx, packet);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                if (mHasAudio) {
                    auto pkt = av_packet_alloc();
                    mAudioPktQueueRef->enqueueEnd(pkt);
                }

                if (mHasVideo) {
                    auto pkt = av_packet_alloc();
                    mVideoPktQueueRef->enqueueEnd(pkt);
                }

                return;
            } else {
//                next_log("av_read_frame ret %d, %s, %d, %d", ret, av_err2str(ret), pktCount, __LINE__);

                //todo
                return;
            }
        }

        if (packet->stream_index == audio_stream_index && audio_stream_index >= 0) {

                if (mAudioPktQueueRef->enqueue(packet)) {
                    packet = nullptr;
                    sendMessage(MESSAGE_ID_READ_PKG);
                } else {
                    sendMessageDelay(MESSAGE_ID_SEND_PKG, 100);
                }

        } else if (packet->stream_index == video_stream_index && video_stream_index >= 0) {
                if (mVideoPktQueueRef->enqueue(packet)) {
                    packet = nullptr;
                    sendMessage(MESSAGE_ID_READ_PKG);
                } else {
                    sendMessageDelay(MESSAGE_ID_SEND_PKG, 100);
                }
        } else {
            av_packet_unref(packet);
            sendMessage(MESSAGE_ID_READ_PKG);
        }
    }
    void Reader::onMessageSendPackage() {
        if (packet->stream_index == audio_stream_index && audio_stream_index >= 0) {

            if (mAudioPktQueueRef->enqueue(packet)) {
                packet = nullptr;
                sendMessage(MESSAGE_ID_READ_PKG);
            } else {
                sendMessageDelay(MESSAGE_ID_SEND_PKG, 100);
            }

        } else if (packet->stream_index == video_stream_index && video_stream_index >= 0) {
            if (mVideoPktQueueRef->enqueue(packet)) {
                packet = nullptr;
                sendMessage(MESSAGE_ID_READ_PKG);
            } else {
                sendMessageDelay(MESSAGE_ID_SEND_PKG, 100);
            }
        } else {
            av_packet_unref(packet);
            sendMessage(MESSAGE_ID_READ_PKG);
        }
    }

    void Reader::open() {
        mContextData = new ReaderContextData();
        int ret = 0;
        ret = avformat_open_input(&(mContextData->fmt_ctx), mPath.c_str(), nullptr, nullptr);
        next_log("avformat_open_input ret %d, %d", ret, __LINE__);
        auto fmt_ctx = mContextData->fmt_ctx;

        ret = avformat_find_stream_info(fmt_ctx, nullptr);
        next_log("avformat_find_stream_info ret %d, %d", ret, __LINE__);

        ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        next_log("av_find_best_stream video ret %d, %d", ret, __LINE__);

        int mediaDuration = 0;
        auto video_stream_index = ret;
        AVStream* video_stream = nullptr;
        AVStream* audio_stream = nullptr;
        if (video_stream_index >= 0) {
            video_stream = fmt_ctx->streams[video_stream_index];
            AVDictionaryEntry *pEntry = av_dict_get(video_stream->metadata, "rotate", nullptr,
                                                    AV_DICT_MATCH_CASE);
            int rotation = 0;
            if (pEntry != nullptr) {
                auto rotationString = pEntry->value;
                if (std::string("90") == rotationString) {
                    rotation = 90;
                } else if (std::string("180") == rotationString) {
                    rotation = 180;
                } else if (std::string("270") == rotationString) {
                    rotation = 270;
                }
            }

            mVideoPktQueueRef->onCodecParametersGot(video_stream->codecpar,
                                                    video_stream->time_base, rotation);


            mediaDuration = av_rescale_q(video_stream->duration,
                                              video_stream->time_base,
                                              AV_TIME_BASE_Q);
            mHasVideo = true;
        }


        ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        next_log("av_find_best_stream audio ret %d, %d", ret, __LINE__);

        auto audio_stream_index = ret;
        if (audio_stream_index >= 0) {
            audio_stream = fmt_ctx->streams[audio_stream_index];
            mAudioPktQueueRef->onCodecParametersGot(audio_stream->codecpar, audio_stream->time_base,
                                                    0);


            mediaDuration = av_rescale_q(audio_stream->duration,
                                              audio_stream->time_base,
                                              AV_TIME_BASE_Q);

            mHasAudio = true;
        }



        mReaderCallback->onDurationKnown(mediaDuration);
        AVPacket * pkt = nullptr;
        auto pktCount = 0;
        while (!isStopped()) {
            int64_t seek = getSeekPosition();
            if (seek != -1) {

                if (video_stream != nullptr) {
                    int64_t start = av_rescale_q(seek,
                                                 AV_TIME_BASE_Q,
                                                 video_stream->time_base);

                    ret = av_seek_frame(fmt_ctx, video_stream_index, start, AVSEEK_FLAG_BACKWARD);
                    if (ret != 0) {
                        throw std::bad_cast();
                    }
                } else if (audio_stream != nullptr) {
                    int64_t start = av_rescale_q(seek,
                                                  AV_TIME_BASE_Q,
                                                  audio_stream->time_base);

                    ret = av_seek_frame(fmt_ctx, audio_stream_index, start, AVSEEK_FLAG_BACKWARD);
                    if (ret != 0) {
                        throw std::bad_cast();
                    }
                }

                mVideoPktQueueRef->clear();
                mAudioPktQueueRef->clear();
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
                if (ret == AVERROR_EOF) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    sendEndPkt();
                    continue;
                } else {
                    next_log("av_read_frame ret %d, %s, %d, %d", ret, av_err2str(ret), pktCount, __LINE__);
                    break;
                }
            }

            pktCount++;
            if (pkt->stream_index == audio_stream_index && audio_stream_index >= 0) {
                while (!isStopped()) {
                    if (mAudioPktQueueRef->enqueue(pkt)) {
                        pkt = nullptr;
                        break;
                    }

                    next_log("enqueue full %d, %d", pktCount, __LINE__);

                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            } else if (pkt->stream_index == video_stream_index && video_stream_index >= 0) {
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
    }

    void Reader::stop() {
        stopped.store(true);
        mThread.stop();
//        mThread->join();
    }

    void Reader::join() {
        mThread.join();
    }

    void Reader::start() {

    }

    void Reader::pause() {

    }

    void Reader::seek() {

    }

    void Reader::sendEndPkt() {

        if (mHasAudio) {
            auto pkt = av_packet_alloc();
            pkt->stream_index = -1;
            while (!isStopped() && !hasSeek()) {
                if (mAudioPktQueueRef->enqueue(pkt)) {
                    pkt = nullptr;
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            if (pkt != nullptr) {
                av_packet_free(&pkt);
            }

            if (isStopped() || hasSeek()) {
                return;
            }
        }

        if (mHasVideo) {
            auto pkt = av_packet_alloc();
            pkt->stream_index = -1;
            while (!isStopped() && !hasSeek()) {
                if (mVideoPktQueueRef->enqueue(pkt)) {
                    pkt = nullptr;
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            if (pkt != nullptr) {
                av_packet_free(&pkt);
            }
        }

    }

    bool Reader::isStopped() {
        return stopped.load();
    }

    void Reader::seekBackward(int64_t newPosition) {
        next_log("seekBackward %ld, %d", newPosition, __LINE__);

        mThread.messageQueue().pushBack(Message(MESSAGE_ID_SEEK).withData2(newPosition));
//        auto seek = newPosition | 0x8000000000000000L;
//        mSeekPosition.store(seek);
    }

    void Reader::seekForward(int64_t newPosition) {
        next_log("seekForward %ld, %d", newPosition, __LINE__);
        mThread.messageQueue().pushBack(Message(MESSAGE_ID_SEEK).withData2(newPosition));
//        auto seek = newPosition | 0x8000000000000000L;
//        mSeekPosition.store(seek);
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

    bool Reader::hasSeek() {
        int64_t seek = mSeekPosition.load();
        if ((seek & 0x8000000000000000L) == 0) {
            return false;
        }

        return true;
    }
}