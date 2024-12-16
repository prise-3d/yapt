//
// Created by franck on 09/06/24.
//

#ifndef YAPT_UTILS_H
#define YAPT_UTILS_H

#include "constants.h"
#include <random>

inline std::mt19937_64& threadGenerator() {
    thread_local std::mt19937_64 generator(std::random_device{}());
    return generator;
}

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.;
}

inline double randomDouble() {
    thread_local static std::uniform_real_distribution<double> distribution(0.0, 1.0);
    return distribution(threadGenerator());
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

inline void randomSeed(uint64_t seed) {
    threadGenerator().seed(seed);
}

inline void randomSeed() {
    std::random_device rd;
    threadGenerator().seed(rd());
}

/**
 * Returns a random real in [min,max).
 * @param min minimum bound (included)
 * @param max maximum bound (excluded)
 * @return a random real in [min,max)
 */
inline double randomDouble(double min, double max) {
    return min + (max - min) * randomDouble();
}

inline int randomInt(int min, int max) {
    // Returns a random integer in [min,max].
    return int(randomDouble(min, max + 1));
}

#endif //YAPT_UTILS_H
