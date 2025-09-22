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


std::vector<unsigned char> energyMap_img5x5 = {
    12,  85, 173,  44, 201,
   190,  33,  67, 142,  58,
    99, 255, 120,  11,  76,
   210, 134,  88,  39, 178,
    55, 199,  24,  61, 144
};

std::vector<unsigned int> min_path_energy_map_img5x5 = {
    12,  85, 173,  44, 201,
   202,  45,  111, 186,  102,
    144, 300, 165,  113,  178,
   354, 278,  201,  152, 291,
    333, 400,  176,  213, 296
};

// Test low_energy_seam
// creation of the minimal seam energy map
TEST(CustomImageFilterTest, MinimalSeamEnergyMap) {

    // init input image (5x5)
    ImageData input(5, 5, 1);
    input.setPixels(energyMap_img5x5.data(), energyMap_img5x5.size());
    
    // Compute minimal energy path map
    std::vector<unsigned int> output = CustomImageFilter::computeMinimalEnergyPathMap(input);

    for (size_t i = 0; i < output.size(); ++i) {
        printf("%u ", output[i]);
        if ((i + 1) % input.getWidth() == 0) printf("\n");
    }
    
    // Check that output matches truth
    auto truth_it = min_path_energy_map_img5x5.begin();
    auto output_it = output.begin();
    for (; truth_it != min_path_energy_map_img5x5.end() && output_it != output.end(); ++truth_it, ++output_it) {
        EXPECT_EQ(*truth_it, *output_it);
    }

}

std::vector<unsigned int> expected_seam_img5x5 = {
    22, // Row 0, Col 2
    18, // Row 1, Col 1
    13, // Row 2, Col 2
    9, // Row 3, Col 2
    3  // Row 4, Col 2
};

// test seam backtracking
// Test low_energy_seam (not implemented yet)
// creation of the minimal seam energy map
TEST(CustomImageFilterTest, SeamDetection) {

    // init input image (5x5)
    ImageData input(5, 5, 1);
    input.setPixels(energyMap_img5x5.data(), energyMap_img5x5.size());

    // Apply minimal energy path map calculation
    std::vector<unsigned int> min_path_energy_map_img5x5 = CustomImageFilter::computeMinimalEnergyPathMap(input);

    // Backtrack the minimal energy seam from the last row
    std::vector<unsigned int> seam(input.getHeight());

    seam = CustomImageFilter::identityMinEnergySeam(min_path_energy_map_img5x5, input.getWidth(), input.getHeight());

    printf("Seam indices (x-coordinates per row):\n");
    for (size_t y = 0; y < seam.size(); ++y) {
        printf("Row %u: Column %u\n",  input.getHeight() - 1 - y, seam[y] % input.getWidth());
    }

    // Check that output matches truth
    auto truth_it = expected_seam_img5x5.begin();
    auto output_it = seam.begin();
    for (; truth_it != expected_seam_img5x5.end() && output_it != seam.end(); ++truth_it, ++output_it) {
        EXPECT_EQ(*truth_it, *output_it);
    }
    
}

// test seam removal
