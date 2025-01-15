//
// Created by linlin zhao on 2024/5/24.
//

#pragma once
#include <thread>
#include <atomic>
#include "LockFrameQueue.h"
#include "MediaClock.h"
#include <aaudio/AAudio.h>
#include "concurrent/MessageThread.h"

extern "C" {
#include "libavutil/rational.h"
}

struct AVCodecContext;
struct AVPacket;
struct AVFrame;

namespace next {

    class VideoPackageQueue;
    class AudioDevice;
    class AudioDataContext;

    class AudioRender : public MessageCallback, MessageThreadCallback {
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
        aaudio_data_callback_result_t audioStream_dataCallback(
                AAudioStream *stream,
                void *audioData,
                int32_t numFrames);

        void preSeek();

        void handleMessage(const next::Message &message) override;
        void onThreadEnded() override;

    private:
        void render();
        void decode(struct AVCodecContext *dec, const struct AVPacket *package, struct AVFrame *videoFrame, int speed);
        void onFrame(AVFrame *frame, AVRational timebase, int speed);


        void onMessageInit();
        void onMessageInitAudioDevice();
        void onMessageProcessPkt();
        void onMessageResendPkt();
        void onMessageReceiveFrame();
        void onMessagePreSeek();
        void onMessagePause();
        void onMessageStart();

        void sendMessage(int id);
        void sendMessage(int id, int priority);
        void sendMessageDelay(int id, int delayMs);

    private:
        friend class AudioDevice;

        MediaClock* mMediaClockRef;
        VideoPackageQueue* mQueueRef;
        AudioDevice* mAudioDevice{nullptr};
        MessageThread mThread;
        LockFrameQueue mFrameQueue;
        struct AVFrame* currentFrame{nullptr};
        std::atomic_bool mStopped{false};
        std::atomic_bool mStreamClosed{false};
        std::atomic_bool mPaused{false};
        AudioDataContext* mDataContext{nullptr};
        struct AVFrame* reusedAudioFrame{nullptr};
    };
}


