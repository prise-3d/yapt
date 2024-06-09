//
// Created by franck on 07/06/24.
//

#ifndef YAPT_RAY_H
#define YAPT_RAY_H

#include "vec3.h"

/**
 * A ray, defined by an point3 origin and a vec3 direction
 * Note that a ray's direction is not necessarily a unit vector
 */
class ray {
public:
    ray();

    ray(const point3 &o, const vec3 &dir);

    [[nodiscard]] point3 at(double t) const;

    [[nodiscard]] const point3 &origin() const;

    [[nodiscard]] const vec3 &direction() const;

private:
    point3 orig;
    vec3 dir;
};

ray shoot_ray(point3 &from, point3 &aiming);

#endif //YAPT_RAY_H
