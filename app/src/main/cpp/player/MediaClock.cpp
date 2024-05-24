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

    void MediaClock::setPts(int64_t pts) {
        mPts.store(pts);
    }

    MediaClock::MediaClock():mPts(0),mStartTime(0) {

    }

    int64_t MediaClock::increasePts(int64_t step) {
        auto previous = mPts.load();
        mPts.store(previous + step);
//        auto previous = mPts.fetch_add(step);
        return previous;
//        return previous + step;
    }

    void MediaClock::reset(int64_t start) {
        mStartTime.store(start);
        mPts.store(0);
    }

    void MediaClock::resetWithPts(int64_t start, int64_t pts) {
        mStartTime.store(start);
        mPts.store(pts);
    }

    void MediaClock::resetStartPts(int64_t pts, int64_t startTime) {
        mStartPts.store(pts);
        mPts.store(pts);
        mStartTime.store(startTime);
    }

    void MediaClock::elapse(int64_t e) {
        
    }

    void MediaClock::calculatePtsWithTime(int64_t now) {
        int64_t last = mStartTime.load();
        auto elapsed = now - last;
        if (elapsed > 0) {
            mPts.store(mStartPts + elapsed);
        }
    }

    void MediaClock::calculatePts() {
        auto now = nowMicro();
        int64_t last = mStartTime.load();
        auto elapsed = now - last;
        if (elapsed > 0) {
            mPts.store(mStartPts + elapsed);
        }
    }
}
