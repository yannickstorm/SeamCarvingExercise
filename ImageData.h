#pragma once
#include <vector>
#include <string>
#include <glad/glad.h>
#include <spdlog/spdlog.h>

class ImageData {
private:

    unsigned int width = 0;     // Image width in pixels
    unsigned int height = 0;    // Image height in pixels
    unsigned int channels = 0;  // Number of channels per pixel (1..4 typical)
    
public:
    std::vector<unsigned char> pixels; // Interleaved pixel buffer

    // Default constructs an 'empty' (0x0x0) image.
    ImageData() : ImageData(0,0,0) {}
    ImageData(unsigned int w, unsigned int h, unsigned int c)
        : width(w), height(h), channels(c) {
            // Allocate & zero-initialize pixel buffer.
            // (Zero fill is useful for predictable initial state / debugging.)
            pixels.resize(w * h * c, 0);
    }

    unsigned int getWidth() const { return width; }
    unsigned int getHeight() const { return height; }
    unsigned int getChannels() const { return channels; }

    void setWidth(unsigned int w) {
        width = w;
        pixels.resize(width * height * channels, 0);
    }
    void setHeight(unsigned int h) {
        height = h;
        pixels.resize(width * height * channels, 0);
    }
    void setChannels(unsigned int c) {
        channels = c;
        pixels.resize(width * height * channels, 0);
    }

    // Translate channel count to an OpenGL format enum suitable for glTexImage2D.
    // Returns 0 on unsupported channel count (caller should handle error).
    GLenum getGLFormat() const {
        switch (channels) {
            case 1: return GL_RED;
            case 2: return GL_RG;
            case 3: return GL_RGB;
            case 4: return GL_RGBA;
            default:
                spdlog::error("Unsupported number of channels: {}", channels);
                return 0; // invalid
        }
    }

    // Assign pixel data from a raw pointer.
    // NOTE: This does NOT validate that 'count' matches width*height*channels.
    void setPixels(const unsigned char* pixels_src, size_t count) {
        // Check for null pointer and zero count
        if(count == 0 || pixels_src == nullptr) {
            pixels.clear();
            return;
        }

        // Set the pixel data
        pixels.assign(pixels_src, pixels_src + count);
    }

    // Raw accessors (mutable / const) for OpenGL texture upload or algorithms
    unsigned char* getPixelData() { return pixels.data(); }
    const unsigned char* getPixelData() const { return pixels.data(); }

    size_t getPixelCount() const { return pixels.size(); }

    /// Print pixel values (debug helper) - heavy for large images.
    void printPixels() const {
        for (size_t i = 0; i < pixels.size(); ++i) {
            printf("%u ", pixels[i]);
            if ((i + 1) % width == 0) printf("\n");
        }
    }
};
