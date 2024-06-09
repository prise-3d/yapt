//
// Created by franck on 08/06/24.
//

#ifndef YAPT_SPHERE_H
#define YAPT_SPHERE_H

#include "vec3.h"
#include "ray.h"
#include "hittable.h"

class sphere : public hittable {
public:
    sphere(const point3 &center, double radius) : center(center), radius(fmax(0, radius)) {}

    bool hit(const ray &r, double tmin, double tmax, hit_record &rec) const override {
        vec3 oc = center - r.origin();
        auto a = r.direction().length2();
        auto h = dot(r.direction(), oc);
        auto c = oc.length2() - radius * radius;

        auto discriminant = h * h - a * c;
        if (discriminant < 0)
            return false;

        auto discriminant_sqrt = sqrt(discriminant);

        // Find the nearest root that lies in the acceptable range.
        auto root = (h - discriminant_sqrt) / a;
        if (root <= tmin || tmax <= root) {
            root = (h + discriminant_sqrt) / a;
            if (root <= tmin || tmax <= root)
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        rec.normal = (rec.p - center) / radius;

        return true;
    }

private:
    point3 center;
    double radius;
};

#endif //YAPT_SPHERE_H
