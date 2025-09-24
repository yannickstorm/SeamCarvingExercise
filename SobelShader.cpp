#include "SobelShader.h"
#include <vector>

const char* SobelShader::vertSrc() {
    return R"(#version 130
in vec2 aPos;
in vec2 aTexCoord;
out vec2 v_texCoord;
void main(){
    v_texCoord = aTexCoord;
    gl_Position = vec4(aPos,0.0,1.0);
})";
}

const char* SobelShader::fragSrc() {
    return R"(#version 130
uniform sampler2D u_image;
in vec2 v_texCoord;
out vec4 fragColor;
void main(){
    float kernelX[9] = float[](
        -1,0,1,
        -2,0,2,
        -1,0,1);
    float kernelY[9] = float[](
        -1,-2,-1,
         0, 0, 0,
         1, 2, 1);
    vec2 texel = 1.0 / textureSize(u_image,0);
    float gx = 0.0; float gy = 0.0; int idx = 0;
    for(int y=-1; y<=1; ++y){
        for(int x=-1; x<=1; ++x){
            float intensity = texture(u_image, v_texCoord + vec2(x,y)*texel).r;
            gx += kernelX[idx]*intensity;
            gy += kernelY[idx]*intensity;
            idx++;
        }
    }
    float mag = length(vec2(gx,gy));
    fragColor = vec4(vec3(mag),1.0);
})";
}

SobelShader::SobelShader(){
    auto compile = [](GLenum type, const char* src){
        GLuint s = glCreateShader(type);
        glShaderSource(s,1,&src,nullptr);
        glCompileShader(s);
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER, vertSrc());
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragSrc());
    sobel_prog = glCreateProgram();
    glAttachShader(sobel_prog, vs);
    glAttachShader(sobel_prog, fs);
    glBindAttribLocation(sobel_prog,0,"aPos");
    glBindAttribLocation(sobel_prog,1,"aTexCoord");
    glLinkProgram(sobel_prog);
    glDeleteShader(vs); glDeleteShader(fs);

    glGenTextures(1,&sobel_input_tex_id);
    glBindTexture(GL_TEXTURE_2D,sobel_input_tex_id);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    glGenTextures(1,&sobel_output_tex);
    glBindTexture(GL_TEXTURE_2D, sobel_output_tex);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1,&sobel_fbo);
}

SobelShader::~SobelShader(){
    if(quadVAO) glDeleteVertexArrays(1,&quadVAO);
    if(quadVBO) glDeleteBuffers(1,&quadVBO);
    if(sobel_prog) glDeleteProgram(sobel_prog);
    if(sobel_fbo) glDeleteFramebuffers(1,&sobel_fbo);
    if(sobel_input_tex_id) glDeleteTextures(1,&sobel_input_tex_id);
}

void SobelShader::renderFullscreenQuad(){
    if(quadVAO==0){
        float quadVertices[] = {
            -1.f, 1.f, 0.f,1.f,
            -1.f,-1.f,0.f,0.f,
             1.f,-1.f,1.f,0.f,
            -1.f, 1.f,0.f,1.f,
             1.f,-1.f,1.f,0.f,
             1.f, 1.f,1.f,1.f};
        glGenVertexArrays(1,&quadVAO);
        glGenBuffers(1,&quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER,sizeof(quadVertices),quadVertices,GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES,0,6);
    glBindVertexArray(0);
}

ImageData SobelShader::apply(const ImageData& image){
    glGenTextures(1,&sobel_output_tex);
    glBindTexture(GL_TEXTURE_2D,sobel_output_tex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,image.getWidth(),image.getHeight(),0,GL_RED,GL_UNSIGNED_BYTE,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D,sobel_input_tex_id);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,image.getWidth(),image.getHeight(),0,GL_RED,GL_UNSIGNED_BYTE,image.getPixelData());

    glBindFramebuffer(GL_FRAMEBUFFER,sobel_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,sobel_output_tex,0);
    glViewport(0,0,image.getWidth(),image.getHeight());
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(sobel_prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,sobel_input_tex_id);
    glUniform1i(glGetUniformLocation(sobel_prog,"u_image"),0);
    renderFullscreenQuad();

    ImageData result(image.getWidth(),image.getHeight(),1);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,image.getWidth(),image.getHeight(),GL_RED,GL_UNSIGNED_BYTE,result.getPixelData());
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    return result;
}
