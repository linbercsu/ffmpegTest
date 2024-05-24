//
// Created by linlin zhao on 2024/5/23.
//

#include "FrameQueue.h"

extern "C" {
#include "libavutil/frame.h"
#include "libavcodec/avcodec.h"
}

namespace next {

    int audioFrameSize(AVFrame* frame) {
        return frame->nb_samples;
    }

    FrameQueue::FrameQueue(int size):mSize(size) {

    }

    FrameQueue::~FrameQueue() {

    }

    void FrameQueue::clear() {
        for (auto frame : mFrameList) {
            auto ptr = frame;
            av_frame_free(&ptr);
        }
        mFrameList.clear();
    }

    bool FrameQueue::isFull() {
        return mCurrentSize >= mSize;
    }

    void FrameQueue::pushFrame(struct AVFrame *frame) {
        mFrameList.emplace_back(frame);

        if (frame->format != AV_PIX_FMT_NONE) {
            auto size = avpicture_get_size((enum AVPixelFormat)frame->format, frame->width, frame->height);
            mCurrentSize += size;
        } else {
            auto size = audioFrameSize(frame);
            mCurrentSize += size;
        }


    }

    struct AVFrame *FrameQueue::pop() {
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

    struct AVFrame * FrameQueue::first() {
        return mFrameList.front();
    }
}
