//
// Created by linlin zhao on 2024/5/23.
//

#include "Player.h"
#include "VideoRender.h"

namespace next {


    void Player::start() {
        mReader.start();
    }

    Player::Player(std::string path): mPath(path), mVideoPackageQueue(), mReader(path, &mVideoPackageQueue) {
        mVideoRender = new VideoRender(&mVideoPackageQueue);
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