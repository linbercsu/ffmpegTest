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
        void onCodecParametersGot(struct AVCodecParameters* parameters, AVRational timebase);
        struct AVCodecParameters* getCodecParameters();
        AVRational getTimebase();
        bool enqueue(struct AVPacket* pkt);
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
        struct AVCodecParameters mCodecParameters;
        std::list<struct AVPacket*> mPktList;
        std::mutex mLock;
        int mSize{0};
        bool needClear{false};
    };
}


