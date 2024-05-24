//
// Created by linlin zhao on 2024/5/23.
//

#include "Player.h"
#include "VideoRender.h"
#include "AudioRender.h"

namespace next {

    Player::Player(std::string path): mPath(path), mVideoPackageQueue(), mAudioPackageQueue(), mReader(path, &mVideoPackageQueue, &mAudioPackageQueue) {
        mVideoRender = new VideoRender(&mVideoPackageQueue, &mMediaClock);
        mAudioRender = new AudioRender(&mAudioPackageQueue, &mMediaClock);
    }

    Player::~Player() {
        delete mAudioRender;
        delete mVideoRender;
    }

    void Player::start() {
        mAudioRender->start();
    }

    void Player::pause() {
        mAudioRender->pause();
    }

    void Player::stop() {
        mReader.stop();
        mAudioRender->stop();
        mVideoRender->stop();
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