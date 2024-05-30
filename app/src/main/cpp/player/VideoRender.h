//
// Created by linlin zhao on 2024/5/23.
//

#pragma once

#include <thread>
#include <list>
#include <mutex>
#include <atomic>
#include <GLES2/gl2.h>
#include "VideoPackageQueue.h"
#include "FrameQueue.h"
#include "MediaClock.h"
#include "BaseEffect.h"
#include "DirectDraw.h"

struct AVCodecContext;
struct AVPacket;
struct AVFrame;

namespace next {

    class DataContext;

    class VideoRender {
    public:
        VideoRender(next::VideoPackageQueue *pQueue, MediaClock* clock);
        ~VideoRender();
        void stop();

        void run();

        void release();

        void updateEffect(int effectIndex);
        void rotate(int rotation);

        void onSurfaceCreated();

        void onDrawFrame();

        void onSurfaceChanged(int w, int h);
    private:
        void runInternal();
        bool isStopped();
        void decode(AVCodecContext *dec, const AVPacket *package, struct AVFrame *frame);

        void onFrame(struct AVFrame *frame, AVRational timebase);
        void render();

        void setCurrentFrame();
        void releaseAndSetCurrentFrame();
    private:
        struct AVFrame* mCurrentFrame{nullptr};
        std::mutex mFrameLock;
        VideoPackageQueue* mQueueRef;
        std::atomic_bool mStopped{false};

        std::thread* mThread{nullptr};
        FrameQueue mFrameQueue;
        MediaClock* mClock;
        GLuint textures[1];
        GLuint texture;
        nx_effect::BaseEffect* effect{nullptr};
        nx_effect::DirectDraw* directDraw{nullptr};
        int mEffectIndex{0};
        int width;
        int height;
        int mRotation{0};
        int mBaseRotation{0};
        struct AVFrame* reusedVideoFrame{nullptr};
        DataContext* mDataContext{nullptr};
    };
}


