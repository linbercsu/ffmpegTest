//
// Created by ZhaoLinlin on 2021/6/11.
//

#ifndef MY_APPLICATION_TESTEFFECT_H
#define MY_APPLICATION_TESTEFFECT_H

#include <stdint.h>
#include <GLES2/gl2.h>
#include "BaseEffect.h"
#include "TextureCopy.h"

namespace nx_effect {
    namespace ns_TestEffect {
        struct Location {
            GLint texture;
            GLint texCoordinate;
            GLint position;
        };
    }

    class TestEffect : public BaseEffect {
    public:
        TestEffect();

        void init() override;
        void draw(int64_t time, GLuint currentTexture, int w, int h) override;

    private:
        void createTexture(int w, int h);
        void copyNewTexture(GLuint target, GLuint currentTexture, int w, int h);
        void doDraw(GLuint pTexture);
    private:
        TextureCopy textureCopy;
        GLuint program;
        GLuint textureArray1[1]{0};
        ns_TestEffect::Location location;
        int64_t startTime;
        int64_t endTime;
        int64_t lastCreateTime{0};
    };
}


#endif //MY_APPLICATION_TESTEFFECT_H
