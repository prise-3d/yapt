//
// Created by franck on 08/06/24.
//
#include "Vec3.h"
#include "ray.h"

Ray::Ray() = default;

Ray::Ray(const Point3 &o, const Vec3 &dir) : orig(o), dir(dir) {}

[[nodiscard]] Point3 Ray::at(double t) const {
    return orig + t * dir;
}

[[nodiscard]] const Point3 &Ray::origin() const {
    return orig;
}

[[nodiscard]] const Vec3 &Ray::direction() const {
    return dir;
}

Ray Ray::shoot(Point3 &from, Point3 &aiming) {
    return {from, aiming - from};
}