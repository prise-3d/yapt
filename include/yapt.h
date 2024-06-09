//
// Created by franck on 09/06/24.
//

#ifndef YAPT_YAPT_H
#define YAPT_YAPT_H

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include "constants.h"

using std::make_shared;
using std::shared_ptr;
using std::sqrt;

// Constants

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.;
}

//inline double random_double() {
//    // Returns a random real in [0,1).
//    return rand() / (RAND_MAX + 1.0);
//}

inline double random_double() {
    static std::uniform_real_distribution<double> distribution(0.0, 1.0);
    static std::mt19937 generator;
    return distribution(generator);
}

/**
 * Returns a random real in [min,max).
 * @param min minimum bound (included)
 * @param max maximum bound (excluded)
 * @return a random real in [min,max)
 */
inline double random_double(double min, double max) {
    return min + (max-min) * random_double();
}



// common headers
#include "vec3.h"
#include "ray.h"
#include "color.h"
#include "interval.h"

#endif //YAPT_YAPT_H
