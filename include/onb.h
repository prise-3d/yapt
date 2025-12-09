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
