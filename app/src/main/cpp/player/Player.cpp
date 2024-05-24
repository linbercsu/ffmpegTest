//
// Created by linlin zhao on 2024/5/23.
//

#include "Player.h"
#include "VideoRender.h"
#include "AudioRender.h"

namespace next {


    void Player::start() {
        mReader.start();
    }

    Player::Player(std::string path): mPath(path), mVideoPackageQueue(), mAudioPackageQueue(), mReader(path, &mVideoPackageQueue, &mAudioPackageQueue) {
        mVideoRender = new VideoRender(&mVideoPackageQueue, &mMediaClock);
        mAudioRender = new AudioRender(&mAudioPackageQueue, &mMediaClock);
    }

    void Player::pause() {
        mReader.pause();
    }

    void Player::stop() {
        mReader.start();
    }

    void Player::seek() {
        mReader.seek();
    }

    //gl thread
    void Player::onSurfaceCreated() {
        mVideoRender->onSurfaceCreated();
    }

    //gl thread
    void Player::onDrawFrame() {
        mVideoRender->onDrawFrame();
    }

    void Player::onSurfaceChanged(int w, int h) {
        mVideoRender->onSurfaceChanged(w, h);
    }
}