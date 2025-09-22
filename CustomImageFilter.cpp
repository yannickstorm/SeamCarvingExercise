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

// Compute the minimal energy path map using dynamic programming
std::vector<unsigned int> CustomImageFilter::computeMinimalEnergyPathMap(ImageData& energyMap) {
    // Create a 2D vector to store the cumulative energy values
    std::vector<unsigned int> minimalEnergyPathMap(energyMap.getWidth() * energyMap.getHeight());

    // Copy first row of energy map to cumulative energy map
    for (unsigned int x = 0; x < energyMap.getWidth(); ++x) {
        minimalEnergyPathMap[x] = static_cast<unsigned int>(energyMap.pixels[x]);
    }

    // Fill in the cumulative energy map
    for (int y = 1; y < energyMap.getHeight(); ++y) {
        for (int x = 0; x < energyMap.getWidth(); ++x) {

            unsigned int idx = y * energyMap.getWidth() + x;
            unsigned int above = (y - 1) * energyMap.getWidth() + x;

            std::vector<unsigned int> candidates = {};
            candidates.push_back(minimalEnergyPathMap[above]); // directly above

            if((x - 1) >= 0) candidates.push_back(minimalEnergyPathMap[above - 1]);
            if((x + 1) < energyMap.getWidth()) candidates.push_back(minimalEnergyPathMap[above + 1]);

            // Find the minimum cumulative energy from the three possible pixels above
            unsigned int minEnergy = *std::min_element(candidates.begin(), candidates.end());

            // Update the cumulative energy for the current pixel
            minimalEnergyPathMap[idx] = static_cast<unsigned int>(energyMap.pixels[idx]) + minEnergy;
        }
    }

    return minimalEnergyPathMap;
}

std::vector<unsigned int> CustomImageFilter::identityMinEnergySeam(const std::vector<unsigned int>& minPathEnergyMap, unsigned int imageWidth, unsigned int imageHeight) {
    // Placeholder for dynamic programming seam calculation implementation

    // Step 3: Remove the seam from the image
    std::vector<unsigned int> carvedPixels;

    // Find the starting point of the minimal energy seam in the last row
    auto lastRowStart = minPathEnergyMap.end() - imageWidth;
    printf("Last row energies: %u\n", *lastRowStart);
    auto minIt = std::min_element(lastRowStart, minPathEnergyMap.end());
    int seamPosX = std::distance(lastRowStart, minIt);
    
    int currRow = imageHeight - 1;
    carvedPixels.push_back(currRow * imageWidth + seamPosX);

    // Follow the seam upwards and store the pixels to be removed
    while(currRow - 1 >= 0) {
        // Mark pixel at (seamX, row) for removal
        // Move to the next row
        unsigned int pixelAbove = (currRow-1) * imageWidth + seamPosX;

        // Determine the next seam position
        std::vector<std::pair<int, unsigned int>> candidates; // pair of (x offset, energy)
        
        unsigned int minNextEnergyPath = minPathEnergyMap[pixelAbove]; // directly above
        char minNextIndex = 0;
        
        if((seamPosX - 1) >= 0){
            if(minPathEnergyMap[pixelAbove - 1] < minNextEnergyPath) {
                minNextEnergyPath = minPathEnergyMap[pixelAbove - 1];
                minNextIndex = -1;
            }
        }
        if((seamPosX + 1) < imageWidth){
            if(minPathEnergyMap[pixelAbove + 1] < minNextEnergyPath) {
                minNextEnergyPath = minPathEnergyMap[pixelAbove + 1];
                minNextIndex = 1;
            }
        }

        // Update seamX to the position of the next pixel in the seam
        seamPosX += minNextIndex;
        carvedPixels.push_back((currRow - 1) * imageWidth + seamPosX);
        currRow--;
    }

    return carvedPixels;
}

