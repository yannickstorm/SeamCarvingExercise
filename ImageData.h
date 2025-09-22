#pragma once
#include <vector>
#include <string>

class ImageData {
private:

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned int format = 0;
    std::vector<unsigned char> pixels;

public:

    ImageData() = default;
    ImageData(int w, int h, int c, unsigned int format)
        : width(w), height(h), channels(c), format(format) {
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getChannels() const { return channels; }
    unsigned int getFormat() const { return format; }

    // Assign pixel data from a raw pointer
    void setPixels(const unsigned char* pixels_src, size_t count) {
        // Check for null pointer and zero count
        if(count == 0 || pixels_src == nullptr) {
            pixels.clear();
            return;
        }

        // Set the pixel data
        pixels.assign(pixels_src, pixels_src + count);
    }

    unsigned char* getPixelData() { return pixels.data(); }
    bool valid() const { return !pixels.empty(); }
};
