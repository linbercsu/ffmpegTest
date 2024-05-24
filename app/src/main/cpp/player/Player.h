//
// Created by linlin zhao on 2024/5/23.
//

#pragma once

#include "Reader.h"
#include "VideoPackageQueue.h"

namespace next {

    class VideoRender;
    class AudioRender;


    class Player {

    public:
        Player(std::string path);
        void start();
        void pause();
        void stop();
        void seek();

        void onSurfaceCreated();

        void onDrawFrame();

        void onSurfaceChanged(int i, int i1);

    private:
        std::string mPath;
        VideoRender* mVideoRender;
        AudioRender* mAudioRender;
        VideoPackageQueue mVideoPackageQueue;
        VideoPackageQueue mAudioPackageQueue;
        Reader mReader;
    };


}

