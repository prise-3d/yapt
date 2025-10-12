//
// Created by franck on 09/06/24.
//

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