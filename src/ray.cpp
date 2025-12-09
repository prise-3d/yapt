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

#include "Vec3.h"
#include "ray.h"

Ray::Ray() = default;

Ray::Ray(const Point3 &o, const Vec3 &dir) : orig(o), dir(dir) {}

[[nodiscard]] Point3 Ray::at(double t) const {
    return orig + t * dir;
}

[[nodiscard]] const Point3 &Ray::origin() const {
    return orig;
}

[[nodiscard]] const Vec3 &Ray::direction() const {
    return dir;
}

Ray Ray::shoot(Point3 &from, Point3 &aiming) {
    return {from, aiming - from};
}