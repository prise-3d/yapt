//
// Created by franck on 07/06/24.
//

#ifndef YAPT_VEC3_H
#define YAPT_VEC3_H

#include "constants.h"
#include <iostream>
#include "utils.h"

class Vec3 {
public:
    double e[3];

    Vec3();

    Vec3(double x, double y, double z);

    [[nodiscard]] double x() const;

    [[nodiscard]] double y() const;

    [[nodiscard]] double z() const;

    Vec3 operator-() const;

    double operator[](int i) const;

    double &operator[](int i);

    Vec3 &operator+=(const Vec3 &v);

    Vec3 &operator*=(double t);

    Vec3 &operator/=(double t);

    [[nodiscard]] double length2() const;

    [[nodiscard]] double length() const;

    [[nodiscard]] bool near_zero() const;


    inline static Vec3 random() {
        return {random_double(), random_double(), random_double()};
    }

    inline static Vec3 random(double min, double max) {
        return {random_double(min, max), random_double(min, max), random_double(min, max)};
    }

    bool operator==(const Vec3& other) const;
};

using Point3 = Vec3;

inline std::ostream &operator<<(std::ostream &out, const Vec3 &v) {
    return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2];
}

inline Vec3 operator+(const Vec3 &u, const Vec3 &v) {
    return {u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]};
}

inline Vec3 operator-(const Vec3 &u, const Vec3 &v) {
    return {u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]};
}

inline Vec3 operator*(const Vec3 &u, const Vec3 &v) {
    return {u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]};
}

inline Vec3 operator*(double t, const Vec3 &v) {
    return {t * v.e[0], t * v[1], t * v[2]};
}

inline Vec3 operator*(const Vec3 &v, double t) {
    return t * v;
}

inline Vec3 operator/(const Vec3 &v, double t) {
    return {v.e[0] / t, v.e[1] / t, v.e[2] / t};
}

inline double dot(const Vec3 &u, const Vec3 &v) {
    return u.e[0] * v.e[0] + u.e[1] * v.e[1] + u.e[2] * v.e[2];
}

inline Vec3 cross(const Vec3 &u, const Vec3 &v) {
    return {u.e[1] * v.e[2] - u.e[2] * v.e[1],
            u.e[2] * v.e[0] - u.e[0] * v.e[2],
            u.e[0] * v.e[1] - u.e[1] * v.e[0]};
}

inline Vec3 unit_vector(const Vec3 &v) {
    return v / v.length();
}

inline Vec3 random_in_unit_sphere() {
    while (true) {
        auto p = Vec3::random(-1, 1);
        if (p.length2() < 1)
            return p;
    }
}

inline Vec3 random_unit_vector() {
    return unit_vector(random_in_unit_sphere());
}

inline Vec3 random_on_hemisphere(const Vec3 &normal) {
    Vec3 on_unit_sphere = random_unit_vector();
    if (dot(on_unit_sphere, normal) > 0.0) // In the same hemisphere as the normal
        return on_unit_sphere;
    else
        return -on_unit_sphere;
}

inline bool is_null(const Vec3 &v) {
    return v.e[0] == 0 && v.e[1] == 0 && v.e[2] == 0;
}

inline bool are_equal(const Vec3 &v, const Vec3 &w) {
    return is_null(v - w);
}

inline bool are_epsilon_equal(const Vec3 &v, const Vec3 &w) {
    return (v - w).length2() < EPSILON;
}

inline Vec3 reflect(const Vec3 &v, const Vec3 &n) {
    return v - 2 * dot(v, n) * n;
}

inline Vec3 refract(const Vec3 &uv, const Vec3 &n, double etai_over_etat) {
    auto cos_theta = fmin(dot(-uv, n), 1.0);
    Vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
    Vec3 r_out_parallel = -sqrt(fabs(1.0 - r_out_perp.length2())) * n;
    return r_out_perp + r_out_parallel;
}

inline Vec3 random_in_unit_disk() {
    while (true) {
        auto p = Vec3(random_double(-1, 1), random_double(-1, 1), 0);
        if (p.length2() < 1)
            return p;
    }
}

inline Vec3 random_cosine_direction() {
    auto r1 = random_double();
    auto r2 = random_double();

    auto phi = 2 * pi * r1;
    auto x = cos(phi) * sqrt(r2);
    auto y = sin(phi) * sqrt(r2);
    auto z = sqrt(1 - r2);

    return {x, y, z};
}

struct Vec3Hash2D {
    std::size_t operator()(const Vec3& p) const {
        std::size_t h1 = std::hash<double>()(p.x());
        std::size_t h2 = std::hash<double>()(p.y());
        return h1 + 37 * h2;
    }
};

#endif //YAPT_VEC3_H
