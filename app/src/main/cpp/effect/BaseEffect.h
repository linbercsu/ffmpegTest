//
// Created by ZhaoLinlin on 2021/6/11.
//

#ifndef MY_APPLICATION_BASEEFFECT_H
#define MY_APPLICATION_BASEEFFECT_H
#include <stdint.h>
#include <GLES2/gl2.h>

namespace nx_effect {
    namespace ns_BaseEffect {
        struct Location {
            GLint texture;
            GLint texCoordinate;
            GLint position;
        };
    }

    class BaseEffect {
    public:
        BaseEffect();

        virtual void init();
        virtual void draw(int64_t time, GLuint currentTexture, int w, int h);

    private:
        void createTexture();
    private:
        GLuint program;
        GLuint textureArray[1];
        ns_BaseEffect::Location location;
        int64_t startTime;
        int64_t endTime;

    };
}
#endif //MY_APPLICATION_BASEEFFECT_H
