//
// Created by linlin zhao on 2024/5/27.
//



#pragma once

#include "ZEffect.h"

namespace nx_effect {

    class MirrorEffect : public ZEffect {
    public:
        MirrorEffect(int rotation);

        void initProgram() override;
    private:
        int mRotation{0};
    };
}

