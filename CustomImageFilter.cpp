#include "CustomImageFilter.h"
#include <algorithm>
#include <spdlog/spdlog.h>

// Sobel Gx (detects vertical edges)
std::vector<int> sobelGx = {
    -1, 0, 1,
    -2, 0, 2,
    -1, 0, 1
};

// Sobel Gy (detects horizontal edges)
std::vector<int> sobelGy = {
    -1, -2, -1,
     0,  0,  0,
     1,  2,  1
};


void convolution(const ImageData& input, ImageData& output, std::vector<int> kernel) {
    output.setWidth(input.getWidth());
    output.setHeight(input.getHeight());
    output.setChannels(input.getChannels());

    unsigned int height = input.getHeight();
    unsigned int width = input.getWidth();


    for (int y = 0; y < input.getHeight(); ++y) {
        for (int x = 0; x < input.getWidth(); ++x) {
            // Apply Sobel filter in X direction
            int convol_res = 0;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int posX = x + kx;
                    int posY = y + ky;

                    // Boundary check. If out of bounds, mirror edge pixels
                    if (posX < 0 || posX >= width) posX = x - kx;
                    if (posY < 0 || posY >= height) posY = y - ky;
                    {
                        /* code */
                    }

                    int pixel = input.pixels[(posY) * width + (posX)];

                    // Apply Sobel filter
                    convol_res += pixel * kernel[(ky + 1) * 3 + (kx + 1)];
                }
            }
            // Absolut value
            convol_res = std::abs(convol_res);
            // Clamp the result to [0, 255]
            convol_res = std::clamp(convol_res, 0, 255);
            output.setPixel(x, y, static_cast<unsigned char>(convol_res));
        }
    }
}

void CustomImageFilter::sobelX(const ImageData& input, ImageData& output) {
    if(input.getChannels() != 1) {
        spdlog::error("SobelX filter only supports single channel images.");
        return;
    }
    convolution(input, output, sobelGx);

}

void CustomImageFilter::sobelY(const ImageData& input, ImageData& output) {
    if(input.getChannels() != 1) {
        spdlog::error("SobelY filter only supports single channel images.");
        return;
    }
     convolution(input, output, sobelGy);

}

// Convert input image to greyscale
void CustomImageFilter::toGreyscale(const ImageData& input, ImageData& output) {
    // Ensure output is sized and formatted correctly
    output = ImageData(input.getWidth(), input.getHeight(), 1);

    auto inIt = input.pixels.begin();
    auto outIt = output.pixels.begin();
    while ( (inIt + input.getChannels() - 1) < input.pixels.end() ) {
        float grey = 0.299f * (*inIt) + 0.587f * (*(inIt + 1)) + 0.114f * (*(inIt + 2));
        *outIt = static_cast<unsigned char>(grey);
        inIt += input.getChannels();
        ++outIt;
    }
}

// Combined Sobel filter (magnitude of both directions)
void CustomImageFilter::sobel(const ImageData& input, ImageData& output) {
    if(input.getChannels() != 1) {
        spdlog::error("Sobel filter only supports single channel images.");
        return;
    }

    ImageData gradX, gradY;
    sobelX(input, gradX);
    sobelY(input, gradY);

    output.setWidth(input.getWidth());
    output.setHeight(input.getHeight());
    output.setChannels(1);

    for (size_t i = 0; i < output.pixels.size(); ++i) {
        int magnitude = static_cast<int>(std::sqrt(gradX.pixels[i] * gradX.pixels[i] + gradY.pixels[i] * gradY.pixels[i]));
        output.pixels[i] = static_cast<unsigned char>(std::clamp(magnitude, 0, 255));
    }
}