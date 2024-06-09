//
// Created by franck on 08/06/24.
//
#include "vec3.h"
#include "ray.h"

ray::ray() = default;

ray::ray(const point3 &o, const vec3 &dir) : orig(o), dir(dir) {}

[[nodiscard]] point3 ray::at(double t) const {
    return orig + t * dir;
}

[[nodiscard]] const point3 &ray::origin() const {
    return orig;
}

[[nodiscard]] const vec3 &ray::direction() const {
    return dir;
}

ray shoot_ray(point3 &from, point3 &aiming) {
    return {from, aiming - from};
}