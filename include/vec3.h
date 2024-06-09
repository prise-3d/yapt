//
// Created by franck on 07/06/24.
//

#ifndef YAPT_VEC3_H
#define YAPT_VEC3_H

#include "constants.h"
#include <iostream>
#include "utils.h"

class vec3 {
public:
    double e[3];

    vec3();

    vec3(double x, double y, double z);

    [[nodiscard]] double x() const;

    [[nodiscard]] double y() const;

    [[nodiscard]] double z() const;

    vec3 operator-() const;

    double operator[](int i) const;

    double &operator[](int i);

    vec3 &operator+=(const vec3 &v);

    vec3 &operator*=(double t);

    vec3 &operator/=(double t);

    [[nodiscard]] double length2() const;

    [[nodiscard]] double length() const;

    [[nodiscard]] bool near_zero() const;


    inline static vec3 random() {
        return vec3(random_double(), random_double(), random_double());
    }

    inline static vec3 random(double min, double max) {
        return vec3(random_double(min,max), random_double(min,max), random_double(min,max));
    }
};

using point3 = vec3;

inline std::ostream &operator<<(std::ostream &out, const vec3 &v) {
    return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2];
}

inline vec3 operator+(const vec3 &u, const vec3 &v) {
    return {u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]};
}

inline vec3 operator-(const vec3 &u, const vec3 &v) {
    return {u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]};
}

inline vec3 operator*(const vec3 &u, const vec3 &v) {
    return {u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]};
}

inline vec3 operator*(double t, const vec3 &v) {
    return {t * v.e[0], t * v[1], t * v[2]};
}

inline vec3 operator*(const vec3 &v, double t) {
    return t * v;
}

inline vec3 operator/(const vec3 &v, double t) {
    return {v.e[0] / t, v.e[1] / t, v.e[2] / t};
}

inline double dot(const vec3 &u, const vec3 &v) {
    return u.e[0] * v.e[0] + u.e[1] * v.e[1] + u.e[2] * v.e[2];
}

inline vec3 cross(const vec3 &u, const vec3 &v) {
    return {u.e[1] * v.e[2] - u.e[2] * v.e[1],
            u.e[2] * v.e[0] - u.e[0] * v.e[2],
            u.e[0] * v.e[1] - u.e[1] * v.e[0]};
}

inline vec3 unit_vector(const vec3 &v) {
    return v / v.length();
}

inline vec3 random_in_unit_sphere() {
    while(true) {
        auto p = vec3::random(-1, 1);
        if (p.length2() < 1)
            return p;
    }
}

inline vec3 random_unit_vector() {
    return unit_vector(random_in_unit_sphere());
}

inline vec3 random_on_hemisphere(const vec3& normal) {
    vec3 on_unit_sphere = random_unit_vector();
    if (dot(on_unit_sphere, normal) > 0.0) // In the same hemisphere as the normal
        return on_unit_sphere;
    else
        return -on_unit_sphere;
}

inline bool is_null(const vec3 &v) {
    return v.e[0] == 0 && v.e[1] == 0 && v.e[2] == 0;
}

inline bool are_equal(const vec3 &v, const vec3 &w) {
    return is_null(v - w);
}

inline bool are_epsilon_equal(const vec3 &v, const vec3 &w) {
    return (v - w).length2() < EPSILON;
}

inline vec3 reflect(const vec3& v, const vec3& n) {
    return v - 2 * dot(v,n) * n;
}

inline vec3 refract(const vec3& uv, const vec3& n, double etai_over_etat) {
    auto cos_theta = fmin(dot(-uv, n), 1.0);
    vec3 r_out_perp =  etai_over_etat * (uv + cos_theta*n);
    vec3 r_out_parallel = -sqrt(fabs(1.0 - r_out_perp.length2())) * n;
    return r_out_perp + r_out_parallel;
}

inline vec3 random_in_unit_disk() {
    while (true) {
        auto p = vec3(random_double(-1,1), random_double(-1,1), 0);
        if (p.length2() < 1)
            return p;
    }
}

#endif //YAPT_VEC3_H
