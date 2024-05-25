//
// Created by linlin zhao on 2024/5/24.
//

#include "LockFrameQueue.h"

extern "C" {
#include "libavutil/frame.h"
#include "libavcodec/avcodec.h"
}

namespace next {

    namespace {
        int audioFrameSize(AVFrame *frame) {
            return frame->nb_samples;
        }
    }

    LockFrameQueue::LockFrameQueue(int size):mSize(size) {

    }

    void LockFrameQueue::clear() {
        std::lock_guard<std::mutex> l(mLock);
        for (auto frame : mFrameList) {
            auto ptr = frame;
            av_frame_free(&ptr);
        }

        mFrameList.clear();
        mCurrentSize = 0;
    }

    bool LockFrameQueue::isFull() {
        std::lock_guard<std::mutex> l(mLock);
        return mCurrentSize >= mSize;
    }

    void LockFrameQueue::pushFrame(struct AVFrame *frame) {
        std::lock_guard<std::mutex> l(mLock);
        mFrameList.emplace_back(frame);

        if (frame->format != AV_PIX_FMT_NONE) {
            auto size = avpicture_get_size((enum AVPixelFormat)frame->format, frame->width, frame->height);
            mCurrentSize += size;
        } else {
            auto size = audioFrameSize(frame);
            mCurrentSize += size;
        }


    }

    struct AVFrame *LockFrameQueue::pop() {
        std::lock_guard<std::mutex> l(mLock);
        if (mFrameList.empty()) {
            return nullptr;
        }

        auto frame = mFrameList.front();
        mFrameList.pop_front();
        if (frame->format != AV_PIX_FMT_NONE) {
            auto size = avpicture_get_size((enum AVPixelFormat)frame->format, frame->width, frame->height);
            mCurrentSize -= size;
        } else {
            auto size = audioFrameSize(frame);
            mCurrentSize -= size;
        }

        return frame;
    }

    struct AVFrame * LockFrameQueue::first() {
        std::lock_guard<std::mutex> l(mLock);
        return mFrameList.front();
    }
}
