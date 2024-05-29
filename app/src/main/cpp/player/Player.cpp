//
// Created by linlin zhao on 2024/5/23.
//

#include "Player.h"
#include "VideoRender.h"
#include "AudioRender.h"
#include "Log.h"

namespace next {

    Player::Player(std::string path): mPath(path), mVideoPackageQueue(), mAudioPackageQueue(), mReader(path, &mVideoPackageQueue, &mAudioPackageQueue, this) {
        mVideoRender = new VideoRender(&mVideoPackageQueue, &mMediaClock);
        mAudioRender = new AudioRender(&mAudioPackageQueue, &mMediaClock);
    }

    Player::~Player() {
        next_log_tag("player", "delete player. %d", __LINE__);
        if (mVideoRender != nullptr) {
            delete mVideoRender;
        }

        if (mAudioRender != nullptr) {
            delete mAudioRender;
        }

    }

    void Player::start() {
        mAudioRender->start();
    }

    void Player::pause() {
        mAudioRender->pause();
    }

    void Player::stop() {
        mReader.stop();
        if (mAudioRender != nullptr)
            mAudioRender->stop();

        if (mVideoRender != nullptr) {
            mVideoRender->stop();
        }
    }

    void Player::seek(int64_t position) {
        mReader.seek();
    }

    //gl thread
    void Player::onSurfaceCreated() {
        if (mVideoRender != nullptr)
        mVideoRender->onSurfaceCreated();
    }

    //gl thread
    void Player::onDrawFrame() {
        if (mVideoRender != nullptr)
        mVideoRender->onDrawFrame();
    }

    void Player::onSurfaceChanged(int w, int h) {
        if (mVideoRender != nullptr)
        mVideoRender->onSurfaceChanged(w, h);
    }

    void Player::onDurationKnown(int64_t duration) {
        mDuration.store(duration);
    }

    int64_t Player::currentPosition() {
        return mMediaClock.getPts();
    }

    void Player::backward(int64_t duration) {
        int64_t c = currentPosition();
        next_log_tag("player", "backward, current: %ld, %d", c, __LINE__);
        auto newPosition = c - duration;
        if (newPosition < 0) {
            newPosition = 0;
        }
        mReader.seekBackward(newPosition);
    }

    void Player::forward(int64_t duration) {
        auto allDuration = mDuration.load();
        int64_t c = currentPosition();
        auto newPosition = c + duration;
        if (newPosition > allDuration) {
            newPosition = allDuration;
        }

        mReader.seekForward(newPosition);
    }

    void Player::setSpeed(int speed) {
        mMediaClock.setSpeed(speed);
//        int64_t c = currentPosition();
//        mReader.seekForward(c);
    }

    void Player::updateEffect(int effect) {
        mVideoRender->updateEffect(effect);
    }
}