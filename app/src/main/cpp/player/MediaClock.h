//
// Created by linlin zhao on 2024/5/24.
//

#pragma once

#include <stdint.h>
#include <atomic>
#include "define.h"

namespace next {

    class MediaClock {
    public:
        MediaClock();

        void resetStartPts(int64_t pts);
        void calculatePts();
//        void reset(int64_t start);
        int64_t getPts();
        void resetPts(int64_t pts);

        void setSpeed(int speed);
        int getSpeed();
    private:
        std::atomic_int mSpeed{DEFAULT_SPEED};
        std::atomic_int64_t mPts{0};
        std::atomic_int64_t mStartPts{0};
        std::atomic_int64_t mStartTime{0};
    };
}


