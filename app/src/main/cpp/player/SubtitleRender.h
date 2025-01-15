//
// Created by Zhao, Linlin on 11/1/25.
//

#pragma once

#include "MediaClock.h"
#include "concurrent/MessageThread.h"

namespace next {

    class VideoPackageQueue;
    class DataContext;

    class SubtitleRender : public MessageCallback, MessageThreadCallback {

    public:
        SubtitleRender(next::VideoPackageQueue *pQueue, MediaClock* clock);

        void stop();

        void handleMessage(const next::Message &message) override;
        void onThreadEnded() override;

    private:
        void onMessageInit();
        void onMessageRender();

        void sendMessage(int id);
        void sendMessage(int id, int priority);
        void sendMessageDelay(int id, int delayMs);
    private:
        VideoPackageQueue* mQueueRef;
        MediaClock* mMediaClockRef;
        MessageThread mThread;
        DataContext* mDataContext{nullptr};


    };

}
