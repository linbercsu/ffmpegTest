//
// Created by linlin zhao on 2024/5/30.
//

#pragma once

#include "BaseEffect.h"
#include <GLES2/gl2.h>

namespace nx_effect {

    namespace ns_ZEffectDD {
        struct Location {
            GLint texture;
            GLint texCoordinate;
            GLint position;
        };
    }

    class DirectDraw : public BaseEffect {

    public:
        DirectDraw();
        virtual ~DirectDraw();

        void init() override;
        virtual void initProgram();
        void draw(int64_t time, GLuint currentTexture, int w, int h, int i) override;
        void draw(GLuint currentTexture, GLfloat* vertex, GLfloat* texture);
    private:
        void createTexture(int w, int h);

        void copyNewTexture(GLuint target, GLuint currentTexture, int w, int h);

        void doDraw(GLuint pTexture, int i, int i1, int i2);
    protected:
        GLuint program;
        ns_ZEffectDD::Location location;
    };
}


