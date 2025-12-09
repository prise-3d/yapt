/*
* This file is part of the YAPT distribution (https://github.com/prise-3d/yapt).
 * Copyright (c) 2025 PrISE-3D.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * --- ADDITIONAL PERMISSION UNDER GNU GPL VERSION 3 SECTION 7 ---
 *
 * If you modify this Program, or any covered work, by linking or
 * combining it with the Intel Math Kernel Library (MKL) (or a modified
 * version of that library), containing parts covered by the terms of
 * the Intel Simplified Software License, the licensors of this
 * Program grant you additional permission to convey the resulting work.
 */

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
