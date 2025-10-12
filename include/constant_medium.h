//
// Created by franck on 10/06/24.
//

#ifndef YAPT_CONSTANT_MEDIUM_H
#define YAPT_CONSTANT_MEDIUM_H

#include "yapt.h"

#include "hittable.h"
#include "material.h"
#include "texture.h"

class ConstantMedium : public Hittable {
public:
    ConstantMedium(shared_ptr<Hittable> boundary, double density, shared_ptr<Texture> tex)
            : boundary(boundary), neg_inv_density(-1/density),
              phase_function(make_shared<Isotropic>(tex))
    {}

    ConstantMedium(shared_ptr<Hittable> boundary, double density, const Color& albedo)
            : boundary(boundary), neg_inv_density(-1/density),
              phase_function(make_shared<Isotropic>(albedo))
    {}

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override {

        HitRecord rec1, rec2;

        if (!boundary->hit(r, Interval::universe, rec1))
            return false;

        if (!boundary->hit(r, Interval(rec1.t + 0.0001, infinity), rec2))
            return false;

        if (rec1.t < ray_t.min) rec1.t = ray_t.min;
        if (rec2.t > ray_t.max) rec2.t = ray_t.max;

        if (rec1.t >= rec2.t)
            return false;

        if (rec1.t < 0)
            rec1.t = 0;

        auto ray_length = r.direction().length();
        auto distance_inside_boundary = (rec2.t - rec1.t) * ray_length;
        auto hit_distance = neg_inv_density * log(random_double());

        if (hit_distance > distance_inside_boundary)
            return false;

        rec.t = rec1.t + hit_distance / ray_length;
        rec.p = r.at(rec.t);

        rec.normal = Vec3(1, 0, 0);  // arbitrary
        rec.front_face = true;     // also arbitrary

        rec.mat = phase_function->get();

        return true;
    }

    [[nodiscard]] AABB bounding_box() const override { return boundary->bounding_box(); }

private:
    shared_ptr<Hittable> boundary;
    double neg_inv_density;
    shared_ptr<Material> phase_function;
};

#endif //YAPT_CONSTANT_MEDIUM_H
