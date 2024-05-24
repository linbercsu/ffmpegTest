//
// Created by linlin zhao on 2024/5/23.
//

#pragma once
#include <stdint.h>

namespace next {

    class Clock {

    public:
        void display();
        int64_t lastDisplayMicro();
        int64_t nowMicro();

    private:
        int64_t mLastDisplay{0};
    };
}


