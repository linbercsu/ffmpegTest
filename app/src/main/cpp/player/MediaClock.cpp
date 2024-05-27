//
// Created by linlin zhao on 2024/5/24.
//

#include "MediaClock.h"

#include <chrono>

using namespace std::chrono;

namespace next {

    namespace {
        int64_t nowMicro() {
            microseconds ms = duration_cast< microseconds >(
                    system_clock::now().time_since_epoch()
            );

            return ms.count();
        }
    }

    int64_t MediaClock::getPts() {
        return mPts.load();
    }

    MediaClock::MediaClock():mStartTime(0) {

    }


//    void MediaClock::reset(int64_t start) {
//        mStartTime.store(start);
//        mPts.store(0);
//    }

    void MediaClock::resetStartPts(int64_t pts) {
        auto now = nowMicro();
        mStartPts.store(pts);
        mPts.store(pts);
        mStartTime.store(now);
    }

    void MediaClock::calculatePts() {
        auto now = nowMicro();
        int64_t last = mStartTime.load();
        auto elapsed = now - last;
        if (elapsed > 0) {
            auto speed = mSpeed.load();
            mPts.store(mStartPts + elapsed * speed);
        }
    }

    void MediaClock::setSpeed(int speed) {
        mSpeed.store(speed);
    }

    int MediaClock::getSpeed() {
        return mSpeed.load();
    }
}
