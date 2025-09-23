#pragma once
#include "ImageData.h"

class CustomImageFilter {
public:
    // Applies a custom filter to the input image and stores the result in output
    static void sobelX(const ImageData& input, ImageData& output);
    static void sobelY(const ImageData& input, ImageData& output);
    static void toGreyscale(const ImageData& input, ImageData& output);
    static void sobel(const ImageData& input, ImageData& output);
    static std::vector<unsigned int> computeMinimalEnergyPathMap(ImageData& energyMap);

    static std::vector<unsigned int> identityMinEnergySeam(const std::vector<unsigned int>& minPathEnergyMap, unsigned int imageWidth, unsigned int imageHeight);

    static void removeSeam(ImageData& image, const std::vector<unsigned int>& seam);
    static void paintSeam(ImageData& image, const std::vector<unsigned int>& seam);
};
