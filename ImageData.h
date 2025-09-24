#pragma once
#include <vector>
#include <string>

class ImageData {
private:

    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;
    
public:
    std::vector<unsigned char> pixels;

    ImageData() = default;
    ImageData(int w, int h, int c)
        : width(w), height(h), channels(c) {
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

    // Set a single pixel with a grayscale value
    void setPixel(int posX, int posY, unsigned char color) {
        if (!valid() || posX < 0 || posX >= width || posY < 0 || posY >= height) return;

        int index = (posY * width + posX) * channels;
        // Set all channels to the same color for simplicity
        for (int c = 0; c < channels; ++c) {
            pixels[index] = color;
        }
    }

    unsigned char* getPixelData() { return pixels.data(); }
    const unsigned char* getPixelData() const { return pixels.data(); }
    size_t getPixelCount() const { return pixels.size(); }
    bool valid() const { return !pixels.empty(); }

    void printPixels() const {
        for (size_t i = 0; i < pixels.size(); ++i) {
            printf("%u ", pixels[i]);
            if ((i + 1) % width == 0) printf("\n");
        }
    }
};
