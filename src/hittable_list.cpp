//
// Created by franck on 09/06/24.
//

#include "hittable_list.h"
#include "hittable.h"
#include <memory>

using std::shared_ptr;
using std::make_shared;

hittable_list::hittable_list() = default;

hittable_list::hittable_list(const shared_ptr<hittable>& object) { add(object); }

void hittable_list::clear() { objects.clear(); }

void hittable_list::add(const shared_ptr<hittable>& object) { objects.push_back(object); }

bool hittable_list::hit(const ray &r, interval ray_t, hit_record &record) const {
    hit_record temporary_record;
    bool hit_anything = false;
    auto closest_so_far = interval(ray_t.min, ray_t.max);

    for (const auto &object: objects) {
        if (object->hit(r, closest_so_far, temporary_record)) {
            hit_anything = true;
            closest_so_far.max = temporary_record.t;
            record = temporary_record;
        }
    }

    return hit_anything;
}