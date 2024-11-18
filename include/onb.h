//
// Created by franck on 11/06/24.
//

#ifndef YAPT_ONB_H
#define YAPT_ONB_H

#include "yapt.h"

class ONB {
public:
    ONB() = default;

    Vec3 operator[](const int i) const { return axis[i]; }

    Vec3 &operator[](const int i) { return axis[i]; }

    [[nodiscard]] Vec3 u() const { return axis[0]; }

    [[nodiscard]] Vec3 v() const { return axis[1]; }

    [[nodiscard]] Vec3 w() const { return axis[2]; }

    [[nodiscard]] Vec3 local(const double a, const double b, const double c) const {
        return a * u() + b * v() + c * w();
    }

    [[nodiscard]] Vec3 local(const Vec3 &a) const {
        return a.x() * u() + a.y() * v() + a.z() * w();
    }

    void build_from_w(const Vec3 &w) {
        const Vec3 unit_w = unit_vector(w);
        const Vec3 a = (fabs(unit_w.x()) > 0.9) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
        const Vec3 v = unit_vector(cross(unit_w, a));
        const Vec3 u = cross(unit_w, v);
        axis[0] = u;
        axis[1] = v;
        axis[2] = unit_w;
    }

private:
    Vec3 axis[3];
};

#endif //YAPT_ONB_H
