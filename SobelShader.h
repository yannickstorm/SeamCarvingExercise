#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include "ImageData.h"

class SobelShader {
private:
    GLuint quadVAO = 0, quadVBO = 0;
    GLuint sobel_prog = 0;
    GLuint sobel_fbo = 0;
    GLuint sobel_input_tex_id = 0;
    GLuint sobel_output_tex = 0;
    static const char* vertSrc();
    static const char* fragSrc();
    void renderFullscreenQuad();
public:
    SobelShader();
    ~SobelShader();
    ImageData apply(const ImageData& image);
};
