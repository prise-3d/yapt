//
// Created by franck on 08/06/24.
//

#ifndef YAPT_HITTABLE_H
#define YAPT_HITTABLE_H

#include "yapt.h"
#include "aabb.h"

class Material;

class HitRecord {
public:
    Point3 p;
    Vec3 normal;
    shared_ptr<Material> mat;
    double t;
    double u;
    double v;
    bool front_face;

    /**
     * Sets the hit record normal vector
     * @param r  the ray intercepting a surface
     * @param outward_normal the outward normal to the surface the ray hits. @warning outward_normal is assumed to have
     * unit length
     */
    inline void set_face_normal(const Ray &r, const Vec3 &outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class Hittable {
public:
    virtual ~Hittable() = default;

    virtual bool hit(const Ray &r, Interval ray_t, HitRecord &rec) const = 0;

    [[nodiscard]] virtual AABB bounding_box() const = 0;

    [[nodiscard]] virtual double pdfValue(const Point3 &origin, const Vec3 &direction) const {
        return 0.0;
    }

    /**
     * Samples a direction to this hittable from an origin
     * @param origin the origin to sample from
     * @return a direction to the hittable
     */
    [[nodiscard]] virtual Vec3 random(const Point3 &origin) const {
        return {1, 0, 0};
    }
};

class Translate : public Hittable {
public:
    Translate(const shared_ptr<Hittable>& object, const Vec3 &offset)
            : object(object), offset(offset) {
        bbox = object->bounding_box() + offset;
    }

    bool hit(const Ray &r, const Interval ray_t, HitRecord &rec) const override {
        // Move the ray backwards by the offset
        Ray offset_r(r.origin() - offset, r.direction());

        // Determine whether an intersection exists along the offset ray (and if so, where)
        if (!object->hit(offset_r, ray_t, rec))
            return false;

        // Move the intersection point forwards by the offset
        rec.p += offset;

        return true;
    }

    [[nodiscard]] AABB bounding_box() const override { return bbox; }

private:
    shared_ptr<Hittable> object;
    Vec3 offset;
    AABB bbox;
};

class RotateY : public Hittable {
public:
    RotateY(const shared_ptr<Hittable>& object, const double angle) : object(object) {
        const auto radians = degrees_to_radians(angle);
        sin_theta = sin(radians);
        cos_theta = cos(radians);
        bbox = object->bounding_box();

        Point3 min(infinity, infinity, infinity);
        Point3 max(-infinity, -infinity, -infinity);

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                for (int k = 0; k < 2; k++) {
                    const auto x = i * bbox.x.max + (1 - i) * bbox.x.min;
                    const auto y = j * bbox.y.max + (1 - j) * bbox.y.min;
                    const auto z = k * bbox.z.max + (1 - k) * bbox.z.min;

                    auto newx = cos_theta * x + sin_theta * z;
                    auto newz = -sin_theta * x + cos_theta * z;

                    Vec3 tester(newx, y, newz);

                    for (int c = 0; c < 3; c++) {
                        min[c] = fmin(min[c], tester[c]);
                        max[c] = fmax(max[c], tester[c]);
                    }
                }
            }
        }

        bbox = AABB(min, max);
    }

    bool hit(const Ray &r, Interval ray_t, HitRecord &rec) const override {
        // Change the ray from world space to object space
        auto origin = r.origin();
        auto direction = r.direction();

        origin[0] = cos_theta * r.origin()[0] - sin_theta * r.origin()[2];
        origin[2] = sin_theta * r.origin()[0] + cos_theta * r.origin()[2];

        direction[0] = cos_theta * r.direction()[0] - sin_theta * r.direction()[2];
        direction[2] = sin_theta * r.direction()[0] + cos_theta * r.direction()[2];

        Ray rotated_r(origin, direction);

        // Determine whether an intersection exists in object space (and if so, where)
        if (!object->hit(rotated_r, ray_t, rec))
            return false;

        // Change the intersection point from object space to world space
        auto p = rec.p;
        p[0] = cos_theta * rec.p[0] + sin_theta * rec.p[2];
        p[2] = -sin_theta * rec.p[0] + cos_theta * rec.p[2];

        // Change the normal from object space to world space
        auto normal = rec.normal;
        normal[0] = cos_theta * rec.normal[0] + sin_theta * rec.normal[2];
        normal[2] = -sin_theta * rec.normal[0] + cos_theta * rec.normal[2];

        rec.p = p;
        rec.normal = normal;

        return true;
    }

    [[nodiscard]] AABB bounding_box() const override { return bbox; }

private:
    shared_ptr<Hittable> object;
    double sin_theta;
    double cos_theta;
    AABB bbox;
};


#endif //YAPT_HITTABLE_H
