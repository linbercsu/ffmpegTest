//
// Created by linlin zhao on 2024/5/23.
//

#pragma once

#include <thread>
#include <list>
#include <mutex>
#include <GLES2/gl2.h>
#include "VideoPackageQueue.h"
#include "FrameQueue.h"
#include "MediaClock.h"
#include "BaseEffect.h"

struct AVCodecContext;
struct AVPacket;
struct AVFrame;

namespace next {

    class VideoRender {
    public:
        VideoRender(next::VideoPackageQueue *pQueue, MediaClock* clock);

        void run();

        void onSurfaceCreated();

        void onDrawFrame();

        void onSurfaceChanged(int w, int h);

    private:
        void decode(AVCodecContext *dec, const AVPacket *package, struct AVFrame *frame);

        void onFrame(struct AVFrame *frame, AVRational timebase);
        void render();

        void setCurrentFrame();
        void releaseAndSetCurrentFrame();
    private:
        struct AVFrame* mCurrentFrame{nullptr};
        std::mutex mFrameLock;
        VideoPackageQueue* mQueue;

        std::thread* mThread{nullptr};
        FrameQueue mFrameQueue;
        MediaClock* mClock;
        GLuint texture;
        nx_effect::BaseEffect* effect;
        int width;
        int height;
    };
}


