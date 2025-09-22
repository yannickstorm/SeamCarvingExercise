#include <gtest/gtest.h>
#include "CustomImageFilter.h"
#include "ImageData.h"

// TODO not nice
const unsigned int GL_RED = 1;

// 5x5 image with a vertical edge in the center
std::vector<unsigned char> monochrom_vertical_edge_img5x5 = {
    0,   0, 10, 0,  0,
    0,   0, 10, 0,  0,
    0,   0, 10, 0,  0,
    0,   0, 10, 0,  0,
    0,   0, 10, 0,  0
};

// 5x5 result image after applying Sobel X filter on the above image
std::vector<unsigned char> monochrom_vertical_edge_sobelX_img5x5 = {
    0, 40,  0, 40,  0,
    0, 40,  0, 40,  0,
    0, 40,  0, 40,  0,
    0, 40,  0, 40,  0,
    0, 40,  0, 40,  0
};

// 5x5 image with a horizontal edge in the center
std::vector<unsigned char> monochrom_hor_edge_img5x5 = {
    0,   0, 0, 0,  0,
    0,   0, 0, 0,  0,
    10,   10, 10, 10,  10,
    0,   0, 0, 0,  0,
    0,   0, 0, 0,  0
};

// 5x5 result image after applying Sobel Y filter on the above image
std::vector<unsigned char> monochrom_hor_edge_sobelY_img5x5 = {
    0,   0, 0, 0,  0,
    40,   40, 40, 40,  40,
    0,   0, 0, 0,  0,
    40,   40, 40, 40,  40,
    0,   0, 0, 0,  0
};

// Test sobel filter (not implemented yet)
// test sobel convolution in Y direction
// test sobel convolution in both directions
TEST(CustomImageFilterTest, SobelY) {
    // init input image (5x5) with horizontal edge
    ImageData input(5, 5, 1);
    input.setPixels(monochrom_hor_edge_img5x5.data(), monochrom_hor_edge_img5x5.size());

    // Init result image for comparison
    ImageData truth(5, 5, 1);
    truth.setPixels(monochrom_hor_edge_sobelY_img5x5.data(), monochrom_hor_edge_sobelY_img5x5.size());

    // Apply sobel filter on output image
    ImageData output(5, 5, 1);
    CustomImageFilter::sobelY(input, output);

    output.printPixels();

    // Check that output matches truth
    for (size_t i = 0; i < output.getPixelCount(); ++i) {
        EXPECT_EQ(truth.getPixelData()[i], output.getPixelData()[i]);
    }

    // init input image (5x5) with vertical edge
    input.setPixels(monochrom_vertical_edge_img5x5.data(), monochrom_vertical_edge_img5x5.size());

    // Apply sobel filter on output image
    CustomImageFilter::sobelY(input, output);

    // Check that output is zero everywhere (no horizontal edges)
    for (size_t i = 0; i < output.getPixelCount(); ++i) {
        EXPECT_EQ(0, output.getPixelData()[i]);
    }
}

// test sobel convolution in X direction
TEST(CustomImageFilterTest, SobelX) {
    // init input image (5x5) with vertical edge
    ImageData input(5, 5, 1);
    input.setPixels(monochrom_vertical_edge_img5x5.data(), monochrom_vertical_edge_img5x5.size());

    // Init result image for comparison
    ImageData truth(5, 5, 1);
    truth.setPixels(monochrom_vertical_edge_sobelX_img5x5.data(), monochrom_vertical_edge_sobelX_img5x5.size());

    // Apply sobel filter on output image
    ImageData output(5, 5, 1);
    CustomImageFilter::sobelX(input, output);

    output.printPixels();

    // Check that output matches truth
    for (size_t i = 0; i < output.getPixelCount(); ++i) {
        EXPECT_EQ(truth.getPixelData()[i], output.getPixelData()[i]);
    }

    // init input image (5x5) with horizontal edge
    input.setPixels(monochrom_hor_edge_img5x5.data(), monochrom_hor_edge_img5x5.size());

    // Apply sobel filter on output image
    CustomImageFilter::sobelX(input, output);

    // Check that output is zero everywhere (no horizontal edges)
    for (size_t i = 0; i < output.getPixelCount(); ++i) {
        EXPECT_EQ(0, output.getPixelData()[i]);
    }
}



// Test low_energy_seam (not implemented yet)
// test dynamic programming table creation
// test seam backtracking
// test seam removal
