//
// Created by linlin zhao on 2024/5/27.
//

#include "GrayEffect.h"
#include "GLUtil.h"

namespace nx_effect {

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

precision highp float;
const highp vec3 W = vec3(0.2125, 0.7154, 0.0721);

void main()
{
    vec4 mask = texture2D(texture,v_TexCoordinate);
    float luminance = dot(mask.rgb,W);
    gl_FragColor = vec4(vec3(luminance),1.0);
}
)";
    }

    void GrayEffect::initProgram() {
        program = nx_effect::createProgram(gVertexShader, gFragmentShader);
    }
}
