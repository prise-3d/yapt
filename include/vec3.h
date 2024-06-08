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
    double e[3];

    vec3(): e{0, 0, 0} {}
    vec3(double x, double y, double z): e{x, y, z} {}

    [[nodiscard]] double x() const { return e[0]; }
    [[nodiscard]] double y() const { return e[1]; }
    [[nodiscard]] double z() const { return e[2]; }

    vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]);}
    double operator[](int i) const { return e[i]; }
    double& operator[](int i) { return e[i]; }

    vec3& operator+=(const vec3& v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        return *this;
    }

    vec3& operator*=(double t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        return *this;
    }

    vec3& operator/=(double t) {
        e[0] /= t;
        e[1] /= t;
        e[2] /= t;
        return *this;
    }

    [[nodiscard]] double length2() const {
        return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
    }

    [[nodiscard]] double length() const {
        return sqrt(length2());
    }
};

using point3 = vec3;

inline std::ostream& operator <<(std::ostream& out, const vec3& v) {
    return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2];
}

inline vec3 operator+(const vec3& u, const vec3& v) {
    return {u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]};
}

inline vec3 operator-(const vec3& u, const vec3& v) {
    return {u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]};
}

inline vec3 operator*(const vec3& u, const vec3& v) {
    return {u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]};
}

inline vec3 operator*(double t, const vec3& v) {
    return {t * v.e[0], t * v[1], t * v[2]};
}

inline vec3 operator*(const vec3& v, double t) {
    return t * v;
}

inline vec3 operator/(const vec3& v, double t) {
    return {v.e[0] / t, v.e[1] / t, v.e[2] / t};
}

inline double dot(const vec3& u, const vec3& v) {
    return u.e[0] * v.e[0] + u.e[1] * v.e[1] + u.e[2] * v.e[2];
}

inline vec3 cross(const vec3& u, const vec3& v) {
    return {u.e[1] * v.e[2] - u.e[2] * v.e[1],
                u.e[2] * v.e[0] - u.e[0] * v.e[2],
                u.e[0] * v.e[1] - u.e[1] * v.e[0]};
}

inline vec3 normalized(const vec3& v) {
    return v / v.length();
}

inline bool is_null(const vec3& v) {
    return v.e[0] == 0 && v.e[1] == 0 && v.e[2] == 0;
}

inline bool are_equal(const vec3& v, const vec3& w) {
    return is_null(v - w);
}

#endif //YAPT_VEC3_H
