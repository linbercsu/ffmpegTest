//
// Created by ZhaoLinlin on 2021/6/11.
//

#include "TestEffect.h"

#include <GLES2/gl2.h>
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

    const GLfloat gTriangleVertices[] = {-1.f, 1.f, 1.f, 1.f, 1.f, -1.f, -1.f, -1.f
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


    void TestEffect::copyNewTexture(GLuint target, GLuint currentTexture, int w, int h) {
//        glUseProgram(program);

        GLuint fbo;
        glGenFramebuffers(1,&fbo);
        checkGlError("glGenFramebuffers");
/// bind the FBO
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        checkGlError("glBindFramebuffer");
/// attach the source texture to the fbo
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, target, 0);
        checkGlError("glFramebufferTexture2D");

//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE0, pTexture);
//        checkGlError("glBindTexture");

        doDraw(currentTexture);
/// bind the destination texture
//        glBindTexture(GL_TEXTURE_2D, currentTexture);
//        checkGlError("glBindTexture");
/// unbind the FBO
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D,0);

        glDeleteFramebuffers(1, &fbo);
    }
    TestEffect::TestEffect()
            :startTime(0)
            ,endTime(0)
    {

    }

    void TestEffect::draw(int64_t time, GLuint currentTexture, int w, int h) {
        if (lastCreateTime == 0 || time - lastCreateTime > 1000L) {
            lastCreateTime = time;
            if (!textureArray1[0]) {
//                            LOGE("copyNewTexture %ld, %ld, %d, %d", time, lastCreateTime, w, h);
                createTexture(w, h);
            }
//            textureCopy.copy(textureArray1[0], currentTexture);
        }
        textureCopy.copy(textureArray1[0], currentTexture);

        glClear(GL_COLOR_BUFFER_BIT);

//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE0, pTexture);
        checkGlError("glBindTexture");


        doDraw(textureArray1[0]);
//        doDraw(currentTexture);

    }

    void TestEffect::doDraw(GLuint pTexture) {
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
//        glBindTexture(GL_TEXTURE1, 0);
    }

    void TestEffect::init(){
//        glClearColor(1.0f, 1.0f, 0.0f, 0.0f);

        program = nx_effect::createProgram(gVertexShader, gFragmentShader);

//        createTexture();

        location.position = glGetAttribLocation(program, "position");
        location.texCoordinate = glGetAttribLocation(program, "texCoordinate");
        location.texture = glGetUniformLocation(program, "texture");
    }


    void TestEffect::createTexture(int w, int h) {
        glGenTextures(  //创建纹理对象
                1, //产生纹理id的数量
                textureArray1
        );
        glBindTexture(GL_TEXTURE_2D, textureArray1[0]);

        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_MIN_FILTER,GL_NEAREST);//设置MIN 采样方式
        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_MAG_FILTER,GL_LINEAR);//设置MAG采样方式
        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);//设置S轴拉伸方式
        glTexParameterf(GL_TEXTURE_2D,
                        GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);//设置T轴拉伸方式

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        checkGlError("createTexture");
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}
