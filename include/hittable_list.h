//
// Created by franck on 09/06/24.
//

#ifndef YAPT_HITTABLE_LIST_H
#define YAPT_HITTABLE_LIST_H

#include "yapt.h"
#include "hittable.h"
#include <vector>

class hittable_list: public hittable {
public:
    std::vector<shared_ptr<hittable>> objects;

    hittable_list();

    hittable_list(const shared_ptr<hittable>& object);

    void clear();

    void add(const shared_ptr<hittable>& object);

    bool hit(const ray &r, interval ray_t, hit_record &record) const override;

    aabb bounding_box() const override;

    double pdf_value(const point3& origin, const vec3& direction) const override;

    vec3 random(const point3& origin) const override;

private:
    aabb bbox;
};

#endif //YAPT_HITTABLE_LIST_H
