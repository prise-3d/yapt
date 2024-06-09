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
    bool front_face;

    /**
     * Sets the the hit record normal vector
     * @param r  the ray intercepting a surface
     * @param outward_normal the outward normal to the surface the ray hits. @warning outward_normal is assumed to have
     * unit length
     */
    inline void set_face_normal(const ray &r, const vec3 &outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class hittable {
public:
    virtual ~hittable() = default;

    virtual bool hit(const ray &r, double ray_tmin, double ray_tmax, hit_record &rec) const = 0;
};

#endif //YAPT_HITTABLE_H
