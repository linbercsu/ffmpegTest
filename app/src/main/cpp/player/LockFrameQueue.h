//
// Created by linlin zhao on 2024/5/23.
//

#pragma once

#include <list>
#include <mutex>

struct AVFrame;

namespace next {

    class LockFrameQueue {
    public:
        LockFrameQueue(int size);
        void clear();
        bool isFull();
        bool isEmpty() const {
            return mFrameList.empty();
        }
        void pushFrame(struct AVFrame * frame);
        struct AVFrame * pop();
        struct AVFrame * first();

    private:
        std::mutex mLock;
        int mSize;
        int mCurrentSize{0};
        std::list<struct AVFrame *> mFrameList;
    };
}


