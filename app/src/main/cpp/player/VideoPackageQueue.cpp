//
// Created by linlin zhao on 2024/5/23.
//

#include "VideoPackageQueue.h"

extern "C" {
#include "libavformat/avformat.h"
}

namespace next {
    auto max_size = 1024 * 1024 * 10;

    void VideoPackageQueue::onCodecParametersGot(struct AVCodecParameters* parameters, AVRational timebase) {
        std::lock_guard<std::mutex> l(mLock);
        mCodecParametersGot = true;
        mTimeBase = timebase;
        avcodec_parameters_copy(&mCodecParameters, parameters);
    }

    struct AVCodecParameters* VideoPackageQueue::getCodecParameters() {
        std::lock_guard<std::mutex> l(mLock);
        if (!mCodecParametersGot) {
            return nullptr;
        }

        return &mCodecParameters;
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

    struct AVPacket *VideoPackageQueue::getPkt() {
        std::lock_guard<std::mutex> l(mLock);
        if (mPktList.empty()) {
            return nullptr;
        }

        auto pkt = mPktList.front();
        mSize -= pkt->size;
        mPktList.pop_front();

        return pkt;
    }

    void VideoPackageQueue::clear() {
        std::lock_guard<std::mutex> l(mLock);
        for (auto pkt : mPktList) {
            auto ptr = pkt;
            av_packet_unref(ptr);
            av_packet_free(&ptr);
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


}
