//
// Created by franck on 09/06/24.
//

#ifndef YAPT_YAPT_H
#define YAPT_YAPT_H

#include <cmath>
#include <iostream>
#include <memory>
#include "constants.h"

using std::make_shared;
using std::shared_ptr;
using std::sqrt;

// Constants

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.;
}

// common headers
#include "vec3.h"
#include "ray.h"
#include "color.h"

#endif //YAPT_YAPT_H
