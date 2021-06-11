//
// Created by ZhaoLinlin on 2021/6/11.
//

#ifndef MY_APPLICATION_GLUTIL_H
#define MY_APPLICATION_GLUTIL_H
#include <GLES2/gl2.h>
#include <android/log.h>

//#define  LOG_TAG    "GLUtil"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

namespace nx_effect {
    void checkGlError(const char* op);

    GLuint loadShader(GLenum shaderType, const char *pSource);
    GLuint createProgram(const char* pVertexSource, const char* pFragmentSource);
}


#endif //MY_APPLICATION_GLUTIL_H
