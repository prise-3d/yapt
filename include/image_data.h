//
// Created by franck on 17/06/24.
//

#ifndef YAPT_IMAGE_DATA_H
#define YAPT_IMAGE_DATA_H

#include <cstdint>
#include <vector>

struct ImageData {
    int width;
    int height;
    std::vector<double> data;
};

#endif //YAPT_IMAGE_DATA_H
