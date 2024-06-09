//
// Created by franck on 09/06/24.
//

#ifndef YAPT_YAPT_H
#define YAPT_YAPT_H

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>

using std::make_shared;
using std::shared_ptr;
using std::sqrt;

// Constants
const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;
const double EPSILON = 1e-10;

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.;
}

// common headers

#endif //YAPT_YAPT_H
