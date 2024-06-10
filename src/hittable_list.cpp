//
// Created by franck on 09/06/24.
//

#include "hittable_list.h"
#include "hittable.h"
#include <memory>

using std::shared_ptr;
using std::make_shared;

hittable_list::hittable_list() = default;

hittable_list::hittable_list(const shared_ptr<hittable> &object) { add(object); }

void hittable_list::clear() { objects.clear(); }

void hittable_list::add(const shared_ptr<hittable> &object) {
    objects.push_back(object);
    bbox = aabb(bbox, object->bounding_box());
}

bool hittable_list::hit(const ray &r, interval ray_t, hit_record &record) const {
    hit_record temp_rec;
    bool hit_anything = false;
    auto closest_so_far = ray_t.max;

    for (const auto& object : objects) {
        if (object->hit(r, interval(ray_t.min, closest_so_far), temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            record = temp_rec;
        }
    }

    return hit_anything;
}

aabb hittable_list::bounding_box() const { return bbox; }