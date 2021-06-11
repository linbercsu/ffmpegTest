//
// Created by ZhaoLinlin on 2021/6/11.
//

#ifndef MY_APPLICATION_TEXTURECOPY_H
#define MY_APPLICATION_TEXTURECOPY_H

#include <GLES2/gl2.h>

namespace nx_effect {

    namespace {
        struct Location {
            GLint texture;
            GLint texCoordinate;
            GLint position;
        };
    }

    class TextureCopy {

    public:
        void copy(GLuint target, GLuint currentTexture);
        ~TextureCopy();
    private:
        void initialize();
        void draw(GLuint pTexture) const;
    private:
        bool init{false};
        GLuint program;
        GLuint FBO;
        Location location;
    };

}
#endif //MY_APPLICATION_TEXTURECOPY_H
