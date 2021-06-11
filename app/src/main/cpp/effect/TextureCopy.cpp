//
// Created by ZhaoLinlin on 2021/6/11.
//

#include "TextureCopy.h"
#include "GLUtil.h"

#define  LOG_TAG    "TestEffect"

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

//    const GLfloat gTriangleVertices[] = {-1.f, -1.f,
//                                         1.f, -1.f,
//                                         1.f, 1.f,
//                                         -1.f, 1.f
//    };

    const GLfloat gTriangleVertices[] = {-1.f, -1.f, 1.f, -1.f, 1.f, 1.f, -1.f, 1.f
    };

    unsigned int indices[] = {
            0, 1, 2, 3
    };

    const GLfloat gTriangleTextures[] = {0.0f, 0.0f, 1.0f, 0.0f,
                                         1.0f, 1.0f, 0.0f, 1.0f};

//    const GLfloat gTriangleTextures[] = {0.f, 1.0f,
//                                         0.0f, 0.f,
//                                         1.0f, 0.0f,
//                                         1.0f, 1.0f};
}

namespace nx_effect {
    void TextureCopy::draw(GLuint pTexture) const {
        glBindTexture(GL_TEXTURE_2D, pTexture);
        glUseProgram(program);
        nx_effect::checkGlError("glUseProgram");

        glVertexAttribPointer(location.position, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
        checkGlError("glVertexAttribPointer");
        glEnableVertexAttribArray(location.position);
        checkGlError("glEnableVertexAttribArray");

        glEnableVertexAttribArray(location.texCoordinate);
        glVertexAttribPointer(location.texCoordinate, 2, GL_FLOAT, false, 0, gTriangleTextures);

        glUniform1i(location.texture, 0);
        nx_effect::checkGlError("glUniform1i");

        glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, indices);
    }

    void TextureCopy::initialize() {
        program = nx_effect::createProgram(gVertexShader, gFragmentShader);

        location.position = glGetAttribLocation(program, "position");
        location.texCoordinate = glGetAttribLocation(program, "texCoordinate");
        location.texture = glGetUniformLocation(program, "texture");

        glGenFramebuffers(1, &FBO);
    }

    void TextureCopy::copy(GLuint target, GLuint currentTexture) {
        if (!init) {
            init = true;
            initialize();
        }

        checkGlError("glGenFramebuffer");
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        checkGlError("glBindFramebuffer");
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, target, 0);
        checkGlError("glFramebufferTexture2D");

        draw(currentTexture);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    TextureCopy::~TextureCopy() {
        glDeleteFramebuffers(1, &FBO);
    }
}