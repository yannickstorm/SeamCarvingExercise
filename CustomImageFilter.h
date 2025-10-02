#pragma once
#include "ImageData.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

class CustomImageFilter {
public:
    // Applies a custom filter to the input image and stores the result in output
    static ImageData sobelX(const ImageData& input);
    static ImageData sobelY(const ImageData& input);
    static ImageData toGreyscale(const ImageData& input);
    static ImageData sobel(const ImageData& input);
    static std::vector<unsigned int> computeMinimalEnergyPathMap(ImageData& energyMap);

    static std::vector<unsigned int> identityMinEnergySeam(const std::vector<unsigned int>& minPathEnergyMap, unsigned int imageWidth, unsigned int imageHeight);

    static void removeSeam(ImageData& image, const std::vector<unsigned int>& seam);
    static void paintSeam(ImageData& image, const std::vector<unsigned int>& seam);

};
