//
// Created by linlin zhao on 2024/5/24.
//

#pragma once
#include <thread>
#include "FrameQueue.h"

extern "C" {
#include "libavutil/rational.h"
}

struct AVCodecContext;
struct AVPacket;
struct AVFrame;

namespace next {

    class VideoPackageQueue;
    class AudioDevice;

    class AudioRender {
    public:
        AudioRender(next::VideoPackageQueue *pQueue);
        void run();

    private:
        void render();
        void decode(struct AVCodecContext *dec, const struct AVPacket *package, struct AVFrame *videoFrame);
        void onFrame(struct AVFrame *frame, AVRational timebase);
    private:
        VideoPackageQueue* mQueue;
        AudioDevice* mAudioDevice;
        std::thread* mThread{nullptr};
        FrameQueue mFrameQueue;
    };
}


