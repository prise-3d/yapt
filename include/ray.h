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
    ray() {}
    ray(const point3& o, const vec3& dir): orig(o), dir(dir) {}

    point3 at(double t) const {
        return orig + t * dir;
    }

    const point3& origin() {
        return orig;
    }

    const vec3& direction() {
        return dir;
    }

    private:
    point3 orig;
    vec3 dir;
};

#endif //YAPT_RAY_H
