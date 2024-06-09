//
// Created by franck on 08/06/24.
//

#ifndef YAPT_HITTABLE_H
#define YAPT_HITTABLE_H

#include "ray.h"

class hit_record {
public:
    point3 p;
    vec3 normal;
    double t;
};

class hittable {
public:
    virtual ~hittable() = default;

    virtual bool hit(const ray &r, double ray_tmin, double ray_tmax, hit_record &rec) const = 0;
};

#endif //YAPT_HITTABLE_H
