//
// Created by linlin zhao on 2024/5/23.
//

#include "VideoPackageQueue.h"
#include "Log.h"
extern "C" {
#include "libavformat/avformat.h"
}

namespace next {
    auto max_size = 1024 * 1024 * 10;

    void VideoPackageQueue::onCodecParametersGot(struct AVCodecParameters* parameters, AVRational timebase) {
        std::lock_guard<std::mutex> l(mLock);
        mCodecParametersGot = true;
        mTimeBase = timebase;
        avcodec_parameters_copy(mCodecParameters, parameters);
    }

    struct AVCodecParameters* VideoPackageQueue::getCodecParameters() {
        std::lock_guard<std::mutex> l(mLock);
        if (!mCodecParametersGot) {
            return nullptr;
        }

        return mCodecParameters;
    }

    bool VideoPackageQueue::enqueue(AVPacket *pkt) {
        std::lock_guard<std::mutex> l(mLock);
        mEnd = false;
        if (mSize > max_size) {
            return false;
        }

        mSize += pkt->size;
        mPktList.emplace_back(pkt);

        return true;
    }

    AVPacket *VideoPackageQueue::getPkt(bool *cleared) {
        std::lock_guard<std::mutex> l(mLock);

        auto clear = needClear;
        needClear = false;

        if (clear) {
            *cleared = true;
//            bool erase = false;
            while (true) {
//                next_log_tag("queue", "getPkt %d", __LINE__);
                bool find = false;
                for (auto ite = mPktList.begin(); ite != mPktList.end(); ite++) {
                    auto ptr = ite.operator*();
                    if (ptr == nullptr) {
                        find = true;
                        break;
                    }
                }

                if (find) {
                    for (auto ite = mPktList.begin(); ite != mPktList.end();) {
                        auto ptr = ite.operator*();
                        if (ptr == nullptr) {
                            mPktList.erase(ite);
                            break;
                        } else {
                            mSize -= ptr->size;
//                            av_packet_unref(ptr);
                            av_packet_free(&ptr);
                            ite = mPktList.erase(ite);
                        }
                    }
                } else {
                    break;
                }
            }
        }

        if (mPktList.empty()) {
            return nullptr;
        }

        auto pkt = mPktList.front();

        assert(pkt != nullptr);

        mSize -= pkt->size;
        mPktList.pop_front();

        return pkt;
    }

    void VideoPackageQueue::clear() {
        std::lock_guard<std::mutex> l(mLock);
        for (auto pkt : mPktList) {
            auto ptr = pkt;
            if (ptr != nullptr) {
//                av_packet_unref(ptr);
                av_packet_free(&ptr);
            }
        }
        mPktList.clear();
    }

    void VideoPackageQueue::end() {
        std::lock_guard<std::mutex> l(mLock);
        mEnd = true;
    }

    bool VideoPackageQueue::isEnd() {
        std::lock_guard<std::mutex> l(mLock);
        return mEnd;
    }

    AVRational VideoPackageQueue::getTimebase() {
        std::lock_guard<std::mutex> l(mLock);
        return mTimeBase;
    }

    bool VideoPackageQueue::getClearFlagAndClear() {
        std::lock_guard<std::mutex> l(mLock);
        auto clear = needClear;
        needClear = false;

        if (clear) {

//            bool erase = false;
            while (true) {
                bool find = false;
                for (auto ite = mPktList.begin(); ite != mPktList.end(); ite++) {
                    auto ptr = ite.operator*();
                    if (ptr == nullptr) {
                        find = true;
                        break;
                    }
                }

                if (find) {
                    for (auto ite = mPktList.begin(); ite != mPktList.end();) {
                        auto ptr = ite.operator*();
                        if (ptr == nullptr) {
                            mPktList.erase(ite);
                            break;
                        } else {
                            mSize -= ptr->size;
                            av_packet_unref(ptr);
                            av_packet_free(&ptr);
                            ite = mPktList.erase(ite);
                        }
                    }
                } else {
                    break;
                }
            }
            }

        return clear;
    }

    void VideoPackageQueue::setNeedClear() {
        std::lock_guard<std::mutex> l(mLock);
        needClear = true;
        mPktList.emplace_back(nullptr);
    }

    VideoPackageQueue::~VideoPackageQueue() {
        avcodec_parameters_free(&mCodecParameters);
    }

    VideoPackageQueue::VideoPackageQueue() {
        mCodecParameters = avcodec_parameters_alloc();
    }
}
