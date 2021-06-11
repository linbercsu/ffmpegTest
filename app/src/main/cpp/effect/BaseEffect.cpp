//
// Created by ZhaoLinlin on 2021/6/11.
//
#include "BaseEffect.h"

#include <GLES2/gl2.h>
#include "GLUtil.h"

namespace {
    auto gVertexShader = R"(
attribute vec4 position;
attribute vec2 texCoordinate;
varying vec2 v_TexCoordinate;

void main()
{
    v_TexCoordinate = texCoordinate;
    gl_Position = vec4(position.x, position.y, position.z, position[3]);
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

    const GLfloat gTriangleVertices[] = {-0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, -0.5f, -0.5f
    };

    unsigned int indices[] = {
            0, 1, 2, 3
    };

    const GLfloat gTriangleTextures[] = { 0.0f, 0.0f, 1.0f, 0.0f,
                                          1.0f, 1.0f, 0.0f, 1.0f};
}

namespace nx_effect {


    BaseEffect::BaseEffect()
    :startTime(0)
    ,endTime(0)
    {

    }

    void BaseEffect::draw(int64_t time, GLuint currentTexture, int w, int h) {
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        nx_effect::checkGlError("glUseProgram");

        glVertexAttribPointer(location.position, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
        checkGlError("glVertexAttribPointer");
        glEnableVertexAttribArray(location.position);
        checkGlError("glEnableVertexAttribArray");

        glEnableVertexAttribArray(location.texCoordinate);
        glVertexAttribPointer(location.texCoordinate, 2, GL_FLOAT, false, 0, gTriangleTextures);
        glUniform1i(location.texture, 0);

//        glBindTexture(GL_TEXTURE_2D, texture);

        glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, indices);
//            glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
        checkGlError("glDrawArrays");

    }

    void BaseEffect::init() {
//        glClearColor(1.0f, 1.0f, 0.0f, 0.0f);

        program = nx_effect::createProgram(gVertexShader, gFragmentShader);

        createTexture();

        location.position = glGetAttribLocation(program, "position");
        location.texCoordinate = glGetAttribLocation(program, "texCoordinate");
        location.texture = glGetUniformLocation(program, "texture");
    }

    void BaseEffect::createTexture() {
        glGenTextures(  //创建纹理对象
                1, //产生纹理id的数量
                textureArray
        );
        glBindTexture(GL_TEXTURE_2D, textureArray[0]);

        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_MIN_FILTER,GL_NEAREST);//设置MIN 采样方式
        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_MAG_FILTER,GL_LINEAR);//设置MAG采样方式
        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);//设置S轴拉伸方式
        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);//设置T轴拉伸方式

    }
}
