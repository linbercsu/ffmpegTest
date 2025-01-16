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

    class Reader;

    class VideoPackageQueue {
    public:
        VideoPackageQueue();
        ~VideoPackageQueue();
        void onCodecParametersGot(Reader* reader, AVCodecParameters *parameters, AVRational timebase, int rotation);
        struct AVCodecParameters* getCodecParameters();
        int getRotation();
        AVRational getTimebase();
        bool enqueue(Reader* reader, struct AVPacket* pkt);
        bool enqueueEnd(Reader* reader, struct AVPacket* pkt);
        void seek(Reader* reader);
        AVPacket *getPkt(bool *pBoolean);
        int getTopAction();

        void end();
        bool isEnd();
        void clear(Reader* reader);
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
        Reader* currentReader{nullptr};
    };
}


