//
// Created by franck on 07/06/24.
//

#include <cmath>
#include <iostream>
#include "vec3.h"

using std::sqrt;

vec3::vec3() : e{0, 0, 0} {}

vec3::vec3(double x, double y, double z) : e{x, y, z} {}

[[nodiscard]] double vec3::x() const { return e[0]; }

[[nodiscard]] double vec3::y() const { return e[1]; }

[[nodiscard]] double vec3::z() const { return e[2]; }

vec3 vec3::operator-() const { return vec3(-e[0], -e[1], -e[2]); }

double vec3::operator[](int i) const { return e[i]; }

double &vec3::operator[](int i) { return e[i]; }

vec3 &vec3::operator+=(const vec3 &v) {
    e[0] += v.e[0];
    e[1] += v.e[1];
    e[2] += v.e[2];
    return *this;
}

vec3 &vec3::operator*=(double t) {
    e[0] *= t;
    e[1] *= t;
    e[2] *= t;
    return *this;
}

vec3 &vec3::operator/=(double t) {
    e[0] /= t;
    e[1] /= t;
    e[2] /= t;
    return *this;
}

[[nodiscard]] double vec3::length2() const {
    return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
}

[[nodiscard]] double vec3::length() const {
    return sqrt(length2());
}
