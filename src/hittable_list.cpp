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

#include "hittable_list.h"
#include "hittable.h"
#include <memory>

using std::shared_ptr;
using std::make_shared;

HittableList::HittableList() = default;

HittableList::HittableList(const shared_ptr<Hittable> &object) { add(object); }

void HittableList::clear() { objects.clear(); }

void HittableList::add(const shared_ptr<Hittable> &object) {
    objects.push_back(object);
    bbox = AABB(bbox, object->bounding_box());
}

bool HittableList::hit(const Ray &r, Interval ray_t, HitRecord &record) const {
    HitRecord temp_rec;
    bool hit_anything = false;
    auto closest_so_far = ray_t.max;

    for (const auto &object: objects) {
        if (object->hit(r, Interval(ray_t.min, closest_so_far), temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            record = temp_rec;
        }
    }

    return hit_anything;
}

AABB HittableList::bounding_box() const { return bbox; }

double HittableList::pdfValue(const Point3 &origin, const Vec3 &direction) const {
    auto weight = 1.0 / objects.size();
    auto sum = 0.0;

    for (const auto &object: objects)
        sum += weight * object->pdfValue(origin, direction);

    return sum;
}

Vec3 HittableList::random(const Point3 &origin) const {
    auto int_size = int(objects.size());
    return objects[random_int(0, int_size - 1)]->random(origin);
}