//
// Created by franck on 08/06/24.
//

#ifndef YAPT_SPHERE_H
#define YAPT_SPHERE_H

#include <utility>

#include "hittable.h"
#include "yapt.h"
#include "aabb.h"
#include "onb.h"


class Sphere : public Hittable {
public:
    Sphere(const Point3 &center, double radius)
            : center(center), radius(fmax(0, radius)) {
        auto rvec = Vec3(radius, radius, radius);
        bbox = AABB(center - rvec, center + rvec);
    }

    Sphere(const Point3 &center, double radius, shared_ptr<Material> mat)
            : center(center), radius(fmax(0, radius)), mat(mat) {
        auto rvec = Vec3(radius, radius, radius);
        bbox = AABB(center - rvec, center + rvec);
    }

    bool hit(const Ray &r, const Interval ray_t, HitRecord &rec) const override {
        const Vec3 oc = center - r.origin();
        const auto a = r.direction().length2();
        const auto h = dot(r.direction(), oc);
        const auto c = oc.length2() - radius * radius;

        const auto discriminant = h * h - a * c;
        if (discriminant < 0)
            return false;

        auto sqrtd = sqrt(discriminant);

        // Find the nearest root that lies in the acceptable range.
        auto root = (h - sqrtd) / a;
        if (!ray_t.surrounds(root)) {
            root = (h + sqrtd) / a;
            if (!ray_t.surrounds(root))
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        const Vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        get_sphere_uv(outward_normal, rec.u, rec.v);

        if (mat)
            rec.mat = mat->get();

        return true;
    }

    [[nodiscard]] bool has(const Point3 &point) const {
        const auto r = point - center;
        const double radius2 = radius * radius;
        return Interval(radius2 - EPSILON, radius2 + EPSILON).contains(r.length2());
    }

    [[nodiscard]] AABB bounding_box() const override { return bbox; }

    [[nodiscard]] double pdfValue(const Point3 &origin, const Vec3 &direction) const override {
        // This method only works for stationary spheres.

        HitRecord rec;
        if (!this->hit(Ray(origin, direction), Interval(0.001, infinity), rec))
            return 0;

        const auto cos_theta_max = sqrt(1 - radius * radius / (center - origin).length2());
        const auto solid_angle = 2 * pi * (1 - cos_theta_max);

        return 1 / solid_angle;
    }

    [[nodiscard]] Vec3 random(const Point3 &origin) const override {
        Vec3 direction = center - origin;
        auto distance_squared = direction.length2();
        ONB uvw;
        uvw.build_from_w(direction);
        return uvw.local(random_to_sphere(radius, distance_squared));
    }


private:
    Point3 center;
    double radius;
    shared_ptr<Material> mat;
    Vec3 center_vec;
    AABB bbox;

    static void get_sphere_uv(const Point3 &p, double &u, double &v) {
        // p: a given point on the sphere of radius one, centered at the origin.
        // u: returned value [0,1] of angle around the Y axis from X=-1.
        // v: returned value [0,1] of angle from Y=-1 to Y=+1.
        //     <1 0 0> yields <0.50 0.50>       <-1  0  0> yields <0.00 0.50>
        //     <0 1 0> yields <0.50 1.00>       < 0 -1  0> yields <0.50 0.00>
        //     <0 0 1> yields <0.25 0.50>       < 0  0 -1> yields <0.75 0.50>

        auto theta = acos(-p.y());
        auto phi = atan2(-p.z(), p.x()) + pi;

        u = phi / (2 * pi);
        v = theta / pi;
    }

    static Vec3 random_to_sphere(double radius, double distance_squared) {
        auto r1 = random_double();
        auto r2 = random_double();
        auto z = 1 + r2 * (sqrt(1 - radius * radius / distance_squared) - 1);

        auto phi = 2 * pi * r1;
        auto x = cos(phi) * sqrt(1 - z * z);
        auto y = sin(phi) * sqrt(1 - z * z);

        return {x, y, z};
    }
};

#endif //YAPT_SPHERE_H
