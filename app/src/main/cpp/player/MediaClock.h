//
// Created by linlin zhao on 2024/5/24.
//

#pragma once

#include <stdint.h>
#include <atomic>

namespace next {

    class MediaClock {
    public:
        MediaClock();

        void resetStartPts(int64_t pts, int64_t startTime);
        void calculatePtsWithTime(int64_t now);
        void calculatePts();
        void elapse(int64_t e);
        void reset(int64_t start);
        void resetWithPts(int64_t start, int64_t pts);
        int64_t getPts();
        void setPts(int64_t pts);
        int64_t increasePts(int64_t step);

    private:
        std::atomic_int64_t mStartPts;
        std::atomic_int64_t mPts;
        std::atomic_int64_t mStartTime;
    };
}


