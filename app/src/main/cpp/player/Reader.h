//
// Created by linlin zhao on 2024/5/23.
//

#pragma once

#include <string>
#include <thread>
#include "VideoPackageQueue.h"

namespace next {
    class ContextData;
    class VideoPackageQueue;

    class Reader {
    public:
        Reader(std::string path, VideoPackageQueue* videoPackageQueue);

        void run();

        void start();
        void pause();
        void stop();
        void seek();

    private:
        void open();
    private:
        VideoPackageQueue* mVideoPktQueueRef{nullptr};
        bool stopped{false};
        bool paused{true};
        std::thread* mThread{nullptr};
        std::string mPath;
        ContextData* mContextData{nullptr};
    };
}


