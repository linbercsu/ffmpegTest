//
// Created by linlin zhao on 2024/5/26.
//

#pragma once

namespace next {

    template<class T> class Releasable {
    public:
        Releasable(T* ref):mRef(ref) {

        }

        ~Releasable() {
            mRef->release();
        }

    private:
        T* mRef;
    };

}