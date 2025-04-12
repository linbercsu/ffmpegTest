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
        int oneSampleSizeInByte(AVFrame * frame) {
            if (frame->format == AV_SAMPLE_FMT_S16) {
                return  2 * frame->channels;
            } else if (frame->format == AV_SAMPLE_FMT_FLT) {
                return 4 * frame->channels;
            }
            throw std::bad_cast();
        }

        int sampleSize(AVFrame * frame, int count) {
            if (frame->nb_samples == 0) {
                return 0;
            }

            return frame->nb_samples;

//            if (frame->format == AV_SAMPLE_FMT_S16) {
//                return count * 2 * frame->channels;
//            } else if (frame->format == AV_SAMPLE_FMT_FLT) {
//                return count * 4 * frame->channels;
//            }
//            throw std::bad_cast();
        }

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

//        if (frame->format != AV_PIX_FMT_NONE) {
//            auto size = avpicture_get_size((enum AVPixelFormat)frame->format, frame->width, frame->height);
//            mCurrentSize += size;
//        } else {
            auto size = audioFrameSize(frame);
            mCurrentSize += size;
//        }


    }

    int64_t LockFrameQueue::fill(void* output, int sampleCount) {
        std::lock_guard<std::mutex> l(mLock);
        if (mFrameList.empty()) {
            if (mSampleSize != -1) {
                memset(output, 0, mSampleSize * sampleCount);
            }
            return AV_NOPTS_VALUE;
        }

        if (mSampleSize == -1) {
            mSampleSize = oneSampleSizeInByte(mFrameList.front());
        }
        auto sampleSize = mSampleSize;
        auto numBytes = sampleSize * sampleCount;

        auto targetPtr = (char*)output;
        auto targetPosition = 0;

        int64_t pts = AV_NOPTS_VALUE;

        while (true) {
            auto frame = mFrameList.front();

            auto position = frame->width;
            auto remain = frame->nb_samples * sampleSize - position;
            if (remain == 0) {
                auto size = audioFrameSize(frame);
                mCurrentSize -= size;

                mFrameList.pop_front();
                av_frame_free(&frame);

                if (mFrameList.empty()) {
                    break;
                }

                continue;
            }

            if (pts == AV_NOPTS_VALUE) {
                pts = frame->pts;
            }

            auto canWrite = std::min(remain, numBytes);
            memcpy(targetPtr + targetPosition, frame->data[0] + position, canWrite);
            frame->width += canWrite;
            targetPosition += canWrite;
            numBytes -= canWrite;

            if (numBytes == 0) {
                break;
            }

            if (numBytes < 0) {
                throw std::bad_exception();
            }
        }

        return pts;
    }

    struct AVFrame *LockFrameQueue::pop() {
        std::lock_guard<std::mutex> l(mLock);
        if (mFrameList.empty()) {
            return nullptr;
        }

        auto frame = mFrameList.front();
        mFrameList.pop_front();
//        if (frame->format != AV_PIX_FMT_NONE) {
//            auto size = avpicture_get_size((enum AVPixelFormat)frame->format, frame->width, frame->height);
//            mCurrentSize -= size;
//        } else {
            auto size = audioFrameSize(frame);
            mCurrentSize -= size;
//        }

        return frame;
    }

    struct AVFrame * LockFrameQueue::first() {
        std::lock_guard<std::mutex> l(mLock);
        return mFrameList.front();
    }

    void LockFrameQueue::resetMaxSize(int size) {
        std::lock_guard<std::mutex> l(mLock);
        mSize = size;
    }
}
