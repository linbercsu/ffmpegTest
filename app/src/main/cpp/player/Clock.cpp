//
// Created by linlin zhao on 2024/5/23.
//

#include "Clock.h"
#include <chrono>

using namespace std::chrono;

namespace next {

    void Clock::display() {
        mLastDisplay = nowMicro();
    }

    int64_t Clock::lastDisplayMicro() {
        return mLastDisplay;
    }

    int64_t Clock::nowMicro() {
        microseconds ms = duration_cast< microseconds >(
                system_clock::now().time_since_epoch()
        );

        return ms.count();
    }
}
