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

#ifndef YAPT_HITTABLE_LIST_H
#define YAPT_HITTABLE_LIST_H

#include "yapt.h"
#include "hittable.h"
#include <vector>

class HittableList : public Hittable {
public:
    std::vector<shared_ptr<Hittable>> objects;

    HittableList();

    explicit HittableList(const shared_ptr<Hittable> &object);

    void clear();

    void add(const shared_ptr<Hittable> &object);

    bool hit(const Ray &r, Interval ray_t, HitRecord &record) const override;

    [[nodiscard]] AABB bounding_box() const override;

    [[nodiscard]] double pdfValue(const Point3 &origin, const Vec3 &direction) const override;

    [[nodiscard]] Vec3 random(const Point3 &origin) const override;

private:
    AABB bbox;
};

#endif //YAPT_HITTABLE_LIST_H
