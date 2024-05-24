//
// Created by linlin zhao on 2024/5/23.
//

#pragma once

#include <list>

struct AVFrame;

namespace next {

    class FrameQueue {
    public:
        FrameQueue(int size);
        bool isFull();
        bool isEmpty() const {
            return mFrameList.empty();
        }
        void pushFrame(struct AVFrame * frame);
        struct AVFrame * pop();
        struct AVFrame * first();

    private:
        int mSize;
        int mCurrentSize{0};
        std::list<struct AVFrame *> mFrameList;
    };
}


