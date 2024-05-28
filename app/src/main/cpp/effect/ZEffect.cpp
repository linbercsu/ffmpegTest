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
        mTriangleTextures = new GLfloat [8];
        mTriangleTextures[0] = 0.0f;
        mTriangleTextures[1] = 0.0f;
        mTriangleTextures[2] = 1.0f;
        mTriangleTextures[3] = 0.0f;
        mTriangleTextures[4] = 1.0f;
        mTriangleTextures[5] = 1.0f;
        mTriangleTextures[6] = 0.0f;
        mTriangleTextures[7] = 1.0f;
    }

    ZEffect::~ZEffect() {
        delete mTriangleTextures;
    }

    void ZEffect::init() {


        initProgram();
//        createTexture();

        location.position = glGetAttribLocation(program, "position");
        location.texCoordinate = glGetAttribLocation(program, "texCoordinate");
        location.texture = glGetUniformLocation(program, "texture");
    }

    void ZEffect::initProgram() {
        program = nx_effect::createProgram(gVertexShader, gFragmentShader);
    }

    void ZEffect::draw(int64_t time, GLuint currentTexture, int w, int h, int padding) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(1.0f, 1.0f, 0.0f, 0.0f);
        glEnable(GL_DEPTH_TEST);
//        glDepthRangef(1.f, 2.f);
        doDraw(currentTexture, w, h, padding);
    }

    void ZEffect::createTexture(int w, int h) {

    }

    void ZEffect::copyNewTexture(GLuint target, GLuint currentTexture, int w, int h) {

    }

    void ZEffect::doDraw(GLuint pTexture, int w, int h, int padding) {
        glBindTexture(GL_TEXTURE_2D, pTexture);
        glUseProgram(program);
        nx_effect::checkGlError("glUseProgram");

        glVertexAttribPointer(location.position, 3, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
        checkGlError("glVertexAttribPointer");
        glEnableVertexAttribArray(location.position);
        checkGlError("glEnableVertexAttribArray");

        glEnableVertexAttribArray(location.texCoordinate);

        auto realPadding = padding;
        float p1X = realPadding / (float) w;
        float p2X = (w - padding) / (float )w;

        mTriangleTextures[0] = p1X;
        mTriangleTextures[2] = p2X;
        mTriangleTextures[4] = p2X;
        mTriangleTextures[6] = p1X;

        glVertexAttribPointer(location.texCoordinate, 2, GL_FLOAT, false, 0, mTriangleTextures);



        glUniform1i(location.texture, 0);
        nx_effect::checkGlError("glUniform1i");

        glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, indices);
    }
}