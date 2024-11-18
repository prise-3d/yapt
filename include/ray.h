//
// Created by franck on 07/06/24.
//

#ifndef YAPT_RAY_H
#define YAPT_RAY_H

#include "Vec3.h"

/**
 * A ray, defined by a Point3 origin and a Vec3 direction
 * Note that a ray's direction is not necessarily a unit vector
 */
class Ray {
public:
    Ray();

    Ray(const Point3 &o, const Vec3 &dir);

    static Ray shoot(Point3 &from, Point3 &aiming);

    [[nodiscard]] Point3 at(double t) const;

    [[nodiscard]] const Point3 &origin() const;

    [[nodiscard]] const Vec3 &direction() const;

private:
    Point3 orig;
    Vec3 dir;
};

#endif //YAPT_RAY_H
