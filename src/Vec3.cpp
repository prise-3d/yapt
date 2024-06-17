//
// Created by franck on 07/06/24.
//

#include <cmath>
#include "Vec3.h"

using std::sqrt;

Vec3::Vec3() : e{0, 0, 0} {}

Vec3::Vec3(double x, double y, double z) : e{x, y, z} {}

[[nodiscard]] double Vec3::x() const { return e[0]; }

[[nodiscard]] double Vec3::y() const { return e[1]; }

[[nodiscard]] double Vec3::z() const { return e[2]; }

Vec3 Vec3::operator-() const { return {-e[0], -e[1], -e[2]}; }

double Vec3::operator[](int i) const { return e[i]; }

double &Vec3::operator[](int i) { return e[i]; }

Vec3 &Vec3::operator+=(const Vec3 &v) {
    e[0] += v.e[0];
    e[1] += v.e[1];
    e[2] += v.e[2];
    return *this;
}

Vec3 &Vec3::operator*=(double t) {
    e[0] *= t;
    e[1] *= t;
    e[2] *= t;
    return *this;
}

Vec3 &Vec3::operator/=(double t) {
    e[0] /= t;
    e[1] /= t;
    e[2] /= t;
    return *this;
}

[[nodiscard]] double Vec3::length2() const {
    return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
}

[[nodiscard]] double Vec3::length() const {
    return sqrt(length2());
}

[[nodiscard]] bool Vec3::near_zero() const {
    return (fabs(e[0]) < EPSILON) && (fabs(e[1]) < EPSILON) && (fabs(e[2]) < EPSILON);
}
