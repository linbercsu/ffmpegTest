//
// Created by linlin zhao on 2024/5/23.
//

#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <stdint.h>
#include "VideoPackageQueue.h"
#include "concurrent/MessageThread.h"

extern "C" {
#include "libavformat/avformat.h"
}


namespace next {
    class ReaderContextData;
    class VideoPackageQueue;

    class ReaderCallback {
    public:
        virtual void onDurationKnown(int64_t duration) = 0;
    };

    class Reader : public MessageCallback, MessageThreadCallback {
    public:
        Reader(const std::string& path, VideoPackageQueue* videoPackageQueue, VideoPackageQueue* audioPackageQueue, VideoPackageQueue* subtitleQueue, ReaderCallback* callback);
        ~Reader();

        void release();

        void start();
        void pause();
        void stop();
        void join();
        void seek();
        void seekBackward(int64_t newPosition);
        void seekForward(int64_t newPosition);

        void handleMessage(const next::Message &message) override;
        void onThreadEnded() override;
    private:
        void onMessageOpen();
        void onMessageSeek(int64_t seek);
        void onMessageReadPackage();
        void onMessageSendPackage();

        void sendMessage(int id);
        void sendMessageDelay(int id, int delayMs);

        bool isStopped();
    private:
        ReaderCallback* mReaderCallback{nullptr};
        VideoPackageQueue* mVideoPktQueueRef{nullptr};
        VideoPackageQueue* mAudioPktQueueRef{nullptr};
        VideoPackageQueue* mSubtitleQueueRef{nullptr};
        bool paused{true};
        std::atomic_bool stopped{false};

//        std::thread* mThread{nullptr};
        MessageThread mThread;
        std::string mPath;
        ReaderContextData* mContextData{nullptr};
        std::atomic_int64_t mSeekPosition{0};
        bool mHasVideo{false};
        bool mHasAudio{false};
        bool mHasSubtitle{false};
        int video_stream_index{-1};
        int audio_stream_index{-1};
        int subtitle_stream_index{-1};
        AVStream* video_stream{nullptr};
        AVStream* audio_stream{nullptr};
        AVStream* subtitle_stream{nullptr};
        AVPacket* packet{nullptr};
    };
}


