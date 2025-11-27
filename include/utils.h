//
// Created by franck on 09/06/24.
//

#ifndef YAPT_UTILS_H
#define YAPT_UTILS_H

#include "constants.h"
#include <random>


std::mt19937_64& threadGenerator();

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.;
}


double random_double();

inline double rnd_double() {
    return random_double();
}

class Poisson {
public:
    explicit Poisson(const double mean): distribution(std::poisson_distribution<>(mean)) {}
    size_t next() {
        return distribution(threadGenerator());
    }
private:
    std::poisson_distribution<> distribution;
};

void random_seed(uint64_t seed);
void random_seed();

/**
 * Returns a random real in [min,max).
 * @param min minimum bound (included)
 * @param max maximum bound (excluded)
 * @return a random real in [min,max)
 */
double random_double(double min, double max);

int random_int(int min, int max);

#endif //YAPT_UTILS_H
