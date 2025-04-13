//
// Created by Zhao, Linlin on 12/4/25.
//

#include "YUVHelper.h"
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>
#include "GLUtil.h"
#include "Log.h"
#include "lodepng.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

namespace nx_effect {
    namespace {
        void saveImage(const char* filename, const unsigned char
        *image, unsigned width, unsigned height) {
            //Encode the image
            lodepng_encode32_file(filename, image, width, height);
        }
// Shader setup



        // Vertex shader
        const char *vertexShaderSource =
                "attribute vec2 position;\n"
                "attribute vec2 texCoord;\n"
                "varying vec2 TexCoord;\n"
                "\n"
                "void main() {\n"
                "    gl_Position = vec4(position, 0.0, 1.0);\n"
                "    TexCoord = texCoord;\n"
                "}\n";

// Fragment shader
        const char *fragmentShaderSource =
                "precision mediump float;\n"
                "varying vec2 TexCoord;\n"
                "uniform sampler2D yTex;\n"
                "uniform sampler2D uTex;\n"
                "uniform sampler2D vTex;\n"
                "\n"
                "void main() {\n"
                "    float y = texture2D(yTex, TexCoord).r;\n"
                "    float u = texture2D(uTex, TexCoord).r - 0.5;\n"
                "    float v = texture2D(vTex, TexCoord).r - 0.5;\n"
                "    \n"
                "    vec3 rgb;\n"
                "    rgb.r = y + 1.402 * v;\n"
                "    rgb.g = y - 0.344136 * u - 0.714136 * v;\n"
                "    rgb.b = y + 1.772 * u;\n"
                "    \n"
                "    gl_FragColor = vec4(rgb, 1.0);\n"
                "}\n";
    }
// Helper function to check shader compilation
    GLuint compile_shader(GLenum type, const char* source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, NULL);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLchar infoLog[512];
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            next_log("Shader compilation failed: %s\n", infoLog);
            return 0;
        }
        return shader;
    }

// Structure to hold shader program and locations
    class YUVConverter {
    public:
        GLuint program;
        GLuint positionLoc;
        GLuint texCoordLoc;
        GLuint yTexLoc;
        GLuint uTexLoc;
        GLuint vTexLoc;
        GLuint yTex;
        GLuint uTex;
        GLuint vTex;
        GLuint fbo;
        GLuint rgbaTex;
    };

    YUVConverter* create_yuv_converter(int width, int height) {
        YUVConverter* converter = new YUVConverter();

        auto hw = width / 2 + 1;
        // Create shader program
        GLuint vertexShader = compile_shader(GL_VERTEX_SHADER, vertexShaderSource);
        GLuint fragmentShader = compile_shader(GL_FRAGMENT_SHADER, fragmentShaderSource);

        converter->program = glCreateProgram();
        glAttachShader(converter->program, vertexShader);
        glAttachShader(converter->program, fragmentShader);
        glLinkProgram(converter->program);

        GLint success;
        glGetProgramiv(converter->program, GL_LINK_STATUS, &success);
        if (!success) {
            GLchar infoLog[512];
            glGetProgramInfoLog(converter->program, 512, NULL, infoLog);
            next_log("Program linking failed: %s\n", infoLog);
            free(converter);
            return NULL;
        }

        // Get attribute locations
        converter->positionLoc = glGetAttribLocation(converter->program, "position");
        nx_effect::checkGlError("1");
        converter->texCoordLoc = glGetAttribLocation(converter->program, "texCoord");
        nx_effect::checkGlError("2");
        // Get uniform locations
        converter->yTexLoc = glGetUniformLocation(converter->program, "yTex");
        nx_effect::checkGlError("3");
        converter->uTexLoc = glGetUniformLocation(converter->program, "uTex");
        nx_effect::checkGlError("4");
        converter->vTexLoc = glGetUniformLocation(converter->program, "vTex");
        nx_effect::checkGlError("5");

        // Create textures
        glGenTextures(1, &converter->yTex);
        glGenTextures(1, &converter->uTex);
        glGenTextures(1, &converter->vTex);
        glGenTextures(1, &converter->rgbaTex);

        // Setup Y texture
        glBindTexture(GL_TEXTURE_2D, converter->yTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
        nx_effect::checkGlError("6");

        // Setup U texture
        glBindTexture(GL_TEXTURE_2D, converter->uTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width/2, height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
        nx_effect::checkGlError("7");

        // Setup V texture
        glBindTexture(GL_TEXTURE_2D, converter->vTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width/2, height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
        nx_effect::checkGlError("8");
        // Setup RGBA texture

        glBindTexture(GL_TEXTURE_2D, converter->rgbaTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        nx_effect::checkGlError("9");
        // Create FBO
        glGenFramebuffers(1, &converter->fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, converter->fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, converter->rgbaTex, 0);

        // Check FBO status
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            next_log("Framebuffer is not complete: %d\n", status);
            free(converter);
            return NULL;
        }

        // Reset state
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return converter;
    }

    void convert_frame(YUVConverter* converter, AVFrame* frame) {
        int width = frame->width;
        int height = frame->height;
        // Update textures with new frame data
        bool strideMode = false;
        if (frame->width != frame->linesize[0]) {
            strideMode = true;
        }

        if (strideMode) {
            glPixelStorei(GL_UNPACK_ROW_LENGTH, frame->linesize[0]);
        }

        glBindTexture(GL_TEXTURE_2D, converter->yTex);
        nx_effect::checkGlError("111");
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->width, frame->height,
                        GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[0]);
        nx_effect::checkGlError("11");
        if (strideMode) {
            glPixelStorei(GL_UNPACK_ROW_LENGTH, frame->linesize[1]);
        }
        glBindTexture(GL_TEXTURE_2D, converter->uTex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->width/2, frame->height/2,
                        GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[1]);
        nx_effect::checkGlError("12");
        if (strideMode) {
            glPixelStorei(GL_UNPACK_ROW_LENGTH, frame->linesize[2]);
        }
        glBindTexture(GL_TEXTURE_2D, converter->vTex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->width/2, frame->height/2,
                        GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[2]);

        if (strideMode) {
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        }
        // Bind FBO and set viewport
        nx_effect::checkGlError("13");
        glBindFramebuffer(GL_FRAMEBUFFER, converter->fbo);
        nx_effect::checkGlError("14");
        glViewport(0, 0, frame->width, frame->height);

        // Use shader program
        glUseProgram(converter->program);
        nx_effect::checkGlError("15");
        // Set textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, converter->yTex);
        glUniform1i(converter->yTexLoc, 0);
        nx_effect::checkGlError("16");
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, converter->uTex);
        glUniform1i(converter->uTexLoc, 1);
        nx_effect::checkGlError("17");

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, converter->vTex);
        glUniform1i(converter->vTexLoc, 2);
        nx_effect::checkGlError("19");
        // Setup vertices
        float vertices[] = {
                // positions   // texCoords
                -1.0f, -1.0f,  0.0f, 0.0f,
                1.0f, -1.0f,  1.0f, 0.0f,
                -1.0f,  1.0f,  0.0f, 1.0f,
                1.0f,  1.0f,  1.0f, 1.0f
        };

        // Draw quad
        glEnableVertexAttribArray(converter->positionLoc);
        glEnableVertexAttribArray(converter->texCoordLoc);

        glVertexAttribPointer(converter->positionLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), vertices);
        glVertexAttribPointer(converter->texCoordLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), vertices + 2);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // Cleanup state
        glDisableVertexAttribArray(converter->positionLoc);
        glDisableVertexAttribArray(converter->texCoordLoc);

#if 0
        auto buffer = new unsigned char[width * height * 4];
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);


        std::string base = "/sdcard/Download/tmp/";
        saveImage((base + std::to_string(1) + ".png").c_str(), buffer, width, height);


        delete [] buffer;
#endif
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void delete_yuv_converter(YUVConverter* converter) {
        if (converter) {
            glDeleteTextures(1, &converter->yTex);
            glDeleteTextures(1, &converter->uTex);
            glDeleteTextures(1, &converter->vTex);
            glDeleteTextures(1, &converter->rgbaTex);
            glDeleteFramebuffers(1, &converter->fbo);
            glDeleteProgram(converter->program);
            free(converter);
        }
    }


// Initialize shaders and textures


    void YUVHelper::init_shader_and_textures(int width, int height) {
        yuvConverter = create_yuv_converter(width, height);
        rgbaTexture = yuvConverter->rgbaTex;
    }

// Function to update textures with new frame data
    void YUVHelper::update_textures(AVFrame *frame) {
        // Update Y plane
        convert_frame(yuvConverter, frame);
    }

// Function to render the frame
/*
void render_frame() {
    glUseProgram(program);

    // Set texture uniforms
    glUniform1i(glGetUniformLocation(program, "yTex"), 0);
    glUniform1i(glGetUniformLocation(program, "uTex"), 1);
    glUniform1i(glGetUniformLocation(program, "vTex"), 2);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
 */

// Main usage example
    void YUVHelper::process_frame(AVFrame *frame) {
        if (!initialized) {
            init_shader_and_textures(frame->width, frame->height);
            initialized = true;
        }

        update_textures(frame);

//    render_frame();
    }
}
