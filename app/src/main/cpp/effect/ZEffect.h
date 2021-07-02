//
// Created by ZhaoLinlin on 2021/6/16.
//

#ifndef MY_APPLICATION_ZEFFECT_H
#define MY_APPLICATION_ZEFFECT_H

#include "BaseEffect.h"
#include <GLES2/gl2.h>

namespace nx_effect {
    namespace ns_ZEffect {
        struct Location {
            GLint texture;
            GLint texCoordinate;
            GLint position;
        };
    }

    class ZEffect : public BaseEffect {

    public:
        ZEffect();

        void init() override;

        void draw(int64_t time, GLuint currentTexture, int w, int h) override;

    private:
        void createTexture(int w, int h);

        void copyNewTexture(GLuint target, GLuint currentTexture, int w, int h);

        void doDraw(GLuint pTexture);

        GLuint program;
        ns_ZEffect::Location location;
    };
}

#endif //MY_APPLICATION_ZEFFECT_H
