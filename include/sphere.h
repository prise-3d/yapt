//
// Created by franck on 08/06/24.
//

#ifndef YAPT_SPHERE_H
#define YAPT_SPHERE_H

#include <utility>

#include "hittable.h"
#include "yapt.h"
#include "aabb.h"

class sphere : public hittable {
public:
    sphere(const point3 &center, double radius, shared_ptr<material> mat) : center(center), radius(fmax(0, radius)), mat(std::move(mat)) {
        auto rvec = vec3(radius, radius, radius);
        bbox = aabb(center - rvec, center + rvec);
    }

    bool hit(const ray &r, interval ray_t, hit_record &rec) const override {
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
        if (!ray_t.surrounds(root)) {
            root = (h + discriminant_sqrt) / a;
            if (!ray_t.surrounds(root))
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.mat = mat;

        return true;
    }

    bool has(point3 point) {
        auto dx = point.x() - center.x();
        auto dy = point.y() - center.y();
        auto dz = point.z() - center.z();
        return fabs(dx * dx + dy * dy + dz * dz - radius * radius) < EPSILON;
    }

    aabb bounding_box() const override {
        return bbox;
    }

private:
    point3 center;
    double radius;
    shared_ptr<material> mat;
    aabb bbox;
};

#endif //YAPT_SPHERE_H
