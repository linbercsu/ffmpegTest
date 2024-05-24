//
// Created by linlin zhao on 2024/5/23.
//

#include "FrameQueue.h"

extern "C" {
#include "libavutil/frame.h"
#include "libavcodec/avcodec.h"
}

namespace next {
    FrameQueue::FrameQueue(int size):mSize(size) {

    }

    bool FrameQueue::isFull() {
        return mCurrentSize >= mSize;
    }

    void FrameQueue::pushFrame(struct AVFrame *frame) {
        mFrameList.emplace_back(frame);

        auto size = avpicture_get_size((enum AVPixelFormat)frame->format, frame->width, frame->height);
        mCurrentSize += size;
    }

    struct AVFrame *FrameQueue::pop() {
        if (mFrameList.empty()) {
            return nullptr;
        }

        auto frame = mFrameList.front();
        mFrameList.pop_front();
        auto size = avpicture_get_size((enum AVPixelFormat)frame->format, frame->width, frame->height);
        mCurrentSize -= size;

        return frame;
    }

    struct AVFrame * FrameQueue::first() {
        return mFrameList.front();
    }
}
