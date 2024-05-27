//
// Created by linlin zhao on 2024/5/27.
//

#include "MirrorEffect.h"

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

void main()
{
    gl_FragColor = texture2D(texture, vec2(1.0 - v_TexCoordinate.x, v_TexCoordinate.y));
}
)";
    }

    void MirrorEffect::initProgram() {
        program = nx_effect::createProgram(gVertexShader, gFragmentShader);
    }
}
