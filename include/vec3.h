//
// Created by franck on 07/06/24.
//

#ifndef YAPT_VEC3_H
#define YAPT_VEC3_H

#include <cmath>
#include <iostream>

using std::sqrt;

class vec3 {
public:
    constexpr static const double EPSILON = 1e-8;

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

inline bool is_null(const vec3 &v) {
    return v.e[0] == 0 && v.e[1] == 0 && v.e[2] == 0;
}

inline bool are_equal(const vec3 &v, const vec3 &w) {
    return is_null(v - w);
}

inline bool are_epsilon_equal(const vec3 &v, const vec3 &w) {
    return (v - w).length2() < vec3::EPSILON;
}

#endif //YAPT_VEC3_H
