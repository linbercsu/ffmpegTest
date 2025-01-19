//
// Created by Zhao, Linlin on 11/1/25.
//

#pragma once

#include "MediaClock.h"
#include "concurrent/MessageThread.h"
#include <jni.h>

struct AVPacket;
struct AVSubtitle;

namespace next {

    class VideoPackageQueue;
    class DataContext;

    class SubtitleRender : public MessageCallback, MessageThreadCallback {

    public:
        SubtitleRender(JNIEnv *env, jobject javaPlayer, next::VideoPackageQueue *pQueue, MediaClock* clock);

        void stop();

        void handleMessage(const next::Message &message) override;
        void onThreadStarted() override;
        void onThreadEnded() override;

    private:
        void onMessageInit();
        void onMessageRender();
        void onMessageRenderDelay();
        void sendMessage(int id);
        void sendMessage(int id, int priority);
        void sendMessageDelay(int id, int delayMs);

        void displaySubtitle(struct AVSubtitle* subtitle);
    private:
        VideoPackageQueue* mQueueRef;
        MediaClock* mMediaClockRef;
        MessageThread mThread;
        DataContext* mDataContext{nullptr};
        struct AVPacket* package{nullptr};
        JavaVM* jvm{nullptr};
        JNIEnv* jvmEnv{nullptr};
        jobject javaPlayerRef{nullptr};
        jmethodID methodId{nullptr};

    };

}
