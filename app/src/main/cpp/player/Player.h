//
// Created by linlin zhao on 2024/5/23.
//

#pragma once

#include "Reader.h"
#include "VideoPackageQueue.h"
#include "MediaClock.h"
#include <atomic>

namespace next {

    class VideoRender;
    class AudioRender;
    class SubtitleRender;


    class Player : ReaderCallback {

    public:
        Player(std::string path);
        ~Player();
        void start();
        void pause();
        void stop();
        void seek(int64_t position);
        void backward(int64_t duration);
        void forward(int64_t duration);

        void onSurfaceCreated();

        void onDrawFrame();

        void onSurfaceChanged(int i, int i1);

        void onDurationKnown(int64_t duration) override;

        int64_t currentPosition();

        void setSpeed(int speed);
        void updateEffect(int effect);

        void rotate(int rotation);

    private:
        std::string mPath;
        VideoRender* mVideoRender{nullptr};
        AudioRender* mAudioRender{nullptr};
        SubtitleRender* mSubtitleRender{nullptr};
        VideoPackageQueue mVideoPackageQueue;
        VideoPackageQueue mAudioPackageQueue;
        VideoPackageQueue mSubtitlePackageQueue;
        Reader mReader;
        Reader* mExternalSubtitleReader;
        MediaClock mMediaClock;

        std::atomic_int64_t mDuration{0};
    };


}

