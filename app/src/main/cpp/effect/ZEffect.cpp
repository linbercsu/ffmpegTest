//
// Created by ZhaoLinlin on 2021/6/16.
//

#include "ZEffect.h"
#include "GLUtil.h"

#define  LOG_TAG    "ZEffect"

namespace {
    auto gVertexShader = R"(
attribute vec4 position;
attribute vec2 texCoordinate;
varying vec2 v_TexCoordinate;

void main()
{
    v_TexCoordinate = texCoordinate;
    gl_Position = position;
})";

    auto gFragmentShader = R"(
precision mediump float;
uniform sampler2D texture;
varying vec2 v_TexCoordinate;

void main()
{
    gl_FragColor = texture2D(texture, v_TexCoordinate);
}
)";

    const GLfloat gTriangleVertices[] = {-1.f, 1.f, -1.f, 1.f, 1.f, -1.0f, 1.f, -1.f, -1.0f, -1.f, -1.f, -1.0f
    };
//
//    const GLfloat gTriangleVertices[] = {-0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, -0.5f, -0.5f
//    };

    unsigned int indices[] = {
            0, 1, 2, 3
    };

    const GLfloat gTriangleTextures[] = { 0.0f, 0.0f, 1.0f, 0.0f,
                                          1.0f, 1.0f, 0.0f, 1.0f};
}

namespace nx_effect {

    ZEffect::ZEffect() {

    }

    void ZEffect::init() {

        program = nx_effect::createProgram(gVertexShader, gFragmentShader);

//        createTexture();

        location.position = glGetAttribLocation(program, "position");
        location.texCoordinate = glGetAttribLocation(program, "texCoordinate");
        location.texture = glGetUniformLocation(program, "texture");
    }

    void ZEffect::draw(int64_t time, GLuint currentTexture, int w, int h) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(1.0f, 1.0f, 0.0f, 0.0f);
        glEnable(GL_DEPTH_TEST);
//        glDepthRangef(1.f, 2.f);
        doDraw(currentTexture);
    }

    void ZEffect::createTexture(int w, int h) {

    }

    void ZEffect::copyNewTexture(GLuint target, GLuint currentTexture, int w, int h) {

    }

    void ZEffect::doDraw(GLuint pTexture) {
        glBindTexture(GL_TEXTURE_2D, pTexture);
        glUseProgram(program);
        nx_effect::checkGlError("glUseProgram");

        glVertexAttribPointer(location.position, 3, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
        checkGlError("glVertexAttribPointer");
        glEnableVertexAttribArray(location.position);
        checkGlError("glEnableVertexAttribArray");

        glEnableVertexAttribArray(location.texCoordinate);
        glVertexAttribPointer(location.texCoordinate, 2, GL_FLOAT, false, 0, gTriangleTextures);

        glUniform1i(location.texture, 0);
        nx_effect::checkGlError("glUniform1i");

        glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, indices);
    }
}