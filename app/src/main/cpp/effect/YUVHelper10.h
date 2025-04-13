//
// Created by Zhao, Linlin on 13/4/25.
//

#pragma once

#include <GLES2/gl2.h>

struct AVFrame;

namespace nx_effect {

    class YUVHelper10 {
    public:
        void process_frame(AVFrame* frame);
        GLuint getRgbaTexture() const {
            return rgbaTexture;
        }

    private:
        void init_shader_and_textures(int width, int height);

        void update_textures(AVFrame* frame);

        GLuint program, yTex, uTex, vTex;
        GLuint vao, vbo;
        GLuint fbo, rgbaTexture;
        bool initialized{false};
        GLint yTexPos;
        GLint uTexPos;
        GLint vTexPos;
        void* yuvConverter{nullptr};
    };
}


