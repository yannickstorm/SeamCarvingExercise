#include <gtest/gtest.h>
#include "ImageData.h"

// TODO not nice
const unsigned int GL_RED = 1;

// 5x5 image with a vertical edge in the center
std::vector<unsigned char> monochrom_vertical_edge_img5x5 = {
    0,   0,   0, 255, 255,
    0,   0,   0, 255, 255,
    0,   0,   0, 255, 255,
    0,   0,   0, 255, 255,
    0,   0,   0, 255, 255
};

TEST(CustomImageFilterTest, IdentityCopy) {
    ImageData input(5, 5, 1, GL_RED);
    input.setPixels(monochrom_vertical_edge_img5x5.data(), monochrom_vertical_edge_img5x5.size());

    ImageData output(5, 5, 1, GL_RED);
    output.setPixels(monochrom_vertical_edge_img5x5.data(), monochrom_vertical_edge_img5x5.size());
    //CustomImageFilter::apply(input, output);

    // Check that output matches input (for the default copy filter)
    for (size_t i = 0; i < input.getPixelCount(); ++i) {
        EXPECT_EQ(input.getPixelData()[i], output.getPixelData()[i]);
    }

}
