//
// Created by linlin zhao on 2024/5/23.
//

#pragma once

#include <list>
#include <mutex>

extern "C" {
#include "libavcodec/codec_par.h"
#include "libavutil/rational.h"
}

struct AVPacket;

namespace next {

    class VideoPackageQueue {
    public:
        VideoPackageQueue();
        ~VideoPackageQueue();
        void onCodecParametersGot(AVCodecParameters *parameters, AVRational timebase, int i);
        struct AVCodecParameters* getCodecParameters();
        int getRotation();
        AVRational getTimebase();
        bool enqueue(struct AVPacket* pkt);
        bool enqueueEnd(struct AVPacket* pkt);
        void setNeedClear();
        AVPacket *getPkt(bool *pBoolean);

        void end();
        bool isEnd();
        void clear();
        bool getClearFlagAndClear();
    private:
        bool mEnd{false};
        bool mCodecParametersGot{false};
        AVRational mTimeBase;
        int mRotation{0};
        struct AVCodecParameters* mCodecParameters{nullptr};
        std::list<struct AVPacket*> mPktList;
        std::mutex mLock;
        int mSize{0};
        bool needClear{false};
    };
}


