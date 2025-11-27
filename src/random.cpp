//
// Created by franck on 20/11/25.
//

#include "utils.h"

std::mt19937_64& threadGenerator() {
    thread_local std::mt19937_64 generator(std::random_device{}());
    return generator;
}

std::uniform_real_distribution<double>& threadDistribution() {
    thread_local std::uniform_real_distribution<double> distribution(0.0, 1.0);
    return distribution;
}

double random_double() {
    return threadDistribution()(threadGenerator());
}

double random_double(const double min, const double max) {
    std::uniform_real_distribution<double> distribution(min, max);
    return distribution(threadGenerator());
}

int random_int(int min, int max) {
    return static_cast<int>(random_double(min, max + 1));
}

void random_seed(uint64_t seed) {
    threadGenerator().seed(seed);
    threadDistribution().reset();
}

void random_seed() {
    std::random_device rd;
    threadGenerator().seed(rd());
    threadDistribution().reset();
}
