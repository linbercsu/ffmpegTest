//
// Created by linlin zhao on 2024/5/24.
//

#pragma once
#include <thread>
#include <atomic>
#include "LockFrameQueue.h"
#include "MediaClock.h"

extern "C" {
#include "libavutil/rational.h"
}

struct AVCodecContext;
struct AVPacket;
struct AVFrame;

namespace next {

    class VideoPackageQueue;
    class AudioDevice;
    class AudioOutput;
    class AudioDataContext;

    class AudioRender {
    public:
        AudioRender(next::VideoPackageQueue *pQueue, MediaClock* clock);
        ~AudioRender();
        void run();

        void runInternal();
        void release();
        void stop();
        void pause();
        void start();
        bool isPaused();
        bool isStopped();
    private:
        void render();
        void decode(struct AVCodecContext *dec, const struct AVPacket *package, struct AVFrame *videoFrame, int speed);
        void onFrame(AVFrame *frame, AVRational timebase, int speed);
    private:
        MediaClock* mMediaClockRef;
        VideoPackageQueue* mQueueRef;
        AudioDevice* mAudioDevice{nullptr};
        std::thread* mThread{nullptr};
        LockFrameQueue mFrameQueue;
        AudioOutput* mAudioOutput{nullptr};
        std::atomic_bool mStopped{false};
        std::atomic_bool mPaused{false};
        AudioDataContext* mDataContext{nullptr};
        struct AVFrame* reusedAudioFrame{nullptr};
    };
}


