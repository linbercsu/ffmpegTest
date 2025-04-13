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
#include "YUVHelper.h"
#include "YUVHelper10.h"
#include "concurrent/MessageThread.h"

struct AVCodecContext;
struct AVPacket;
struct AVFrame;

namespace next {


    class RenderThread : public MessageThreadCallback, MessageCallback {
    public:
        RenderThread();
        void stop();
        void join();

        void render();
        void onIdle();
        void handleMessage(const next::Message &message) override;
        void onThreadEnded() override;

    private:
        MessageThread mThread;
    };

    class DataContext;

    class VideoRender : public MessageCallback, MessageThreadCallback{
    public:
        VideoRender(next::VideoPackageQueue *pQueue, MediaClock* clock);
        ~VideoRender();
        void stop();

        void release();

        void updateEffect(int effectIndex);
        void rotate(int rotation);

        void preSeek();

        void onSurfaceCreated();

        void onDrawFrame();

        void onSurfaceChanged(int w, int h);

        void handleMessage(const next::Message &message) override;
        void onThreadEnded() override;
    private:
        bool isStopped();

        void onFrame(struct AVFrame *frame, AVRational timebase);

        void setCurrentFrame();
        void releaseAndSetCurrentFrame();

        void onMessageInit();
        void onMessageProcessPkt();
        void onMessageResendPkt();
        void onMessageReceiveFrame();
        void onMessagePreSeek();
        void sendMessage(int id);
        void sendMessage(int id, int priority);
        void sendMessageDelay(int id, int delayMs);

        void prepareFrame();
    private:
        struct AVFrame* mCurrentFrame{nullptr};
        std::mutex mFrameLock;
        VideoPackageQueue* mQueueRef;
        std::atomic_bool mStopped{false};

        MessageThread mThread;
        FrameQueue mFrameQueue;
        MediaClock* mClock;
        GLuint textures[1];
        GLuint texture;
        GLuint fbo;
        GLuint target;
        bool fboProcessed{false};
        nx_effect::BaseEffect* effect{nullptr};
        nx_effect::DirectDraw* directDraw{nullptr};
        int mEffectIndex{0};
        int mNewEffectIndex{0};
        int width;
        int height;
        int mRotation{0};
        int mBaseRotation{0};
        struct AVFrame* reusedVideoFrame{nullptr};
        struct AVFrame* cachedVideoFrame{nullptr};
        struct AVFrame* cached2VideoFrame{nullptr};
        DataContext* mDataContext{nullptr};
        nx_effect::YUVHelper* yuvHelper{nullptr};
        nx_effect::YUVHelper10* yuvHelper10{nullptr};
        int jump{0};
    };
}


