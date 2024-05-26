//
// Created by linlin zhao on 2024/5/23.
//

#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <stdint.h>
#include "VideoPackageQueue.h"

namespace next {
    class ReaderContextData;
    class VideoPackageQueue;

    class ReaderCallback {
    public:
        virtual void onDurationKnown(int64_t duration) = 0;
    };

    class Reader {
    public:
        Reader(std::string path, VideoPackageQueue* videoPackageQueue, VideoPackageQueue* audioPackageQueue, ReaderCallback* callback);
        ~Reader();

        void release();
        void run();

        void start();
        void pause();
        void stop();
        void seek();
        void seekBackward(int64_t newPosition);
        void seekForward(int64_t newPosition);
        int64_t getSeekPosition();
    private:
        void open();
        bool isStopped();
    private:
        ReaderCallback* mReaderCallback{nullptr};
        VideoPackageQueue* mVideoPktQueueRef{nullptr};
        VideoPackageQueue* mAudioPktQueueRef{nullptr};
        bool paused{true};
        std::atomic_bool stopped{false};

        std::thread* mThread{nullptr};
        std::string mPath;
        ReaderContextData* mContextData{nullptr};
        std::atomic_int64_t mSeekPosition{0};
    };
}


