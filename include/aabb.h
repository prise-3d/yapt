//
// Created by franck on 09/06/24.
//

#ifndef YAPT_AABB_H
#define YAPT_AABB_H

#include "yapt.h"

class AABB {
public:
    Interval x, y, z;

    AABB() = default; // The default AABB is empty, since intervals are empty by default.

    AABB(const Interval &x, const Interval &y, const Interval &z)
            : x(x), y(y), z(z) {
        pad_to_minimums();
    }

    AABB(const Point3 &a, const Point3 &b) {
        // Treat the two points a and b as extrema for the bounding box, so we don't require a
        // particular minimum/maximum coordinate order.

        x = (a[0] <= b[0]) ? Interval(a[0], b[0]) : Interval(b[0], a[0]);
        y = (a[1] <= b[1]) ? Interval(a[1], b[1]) : Interval(b[1], a[1]);
        z = (a[2] <= b[2]) ? Interval(a[2], b[2]) : Interval(b[2], a[2]);

        pad_to_minimums();
    }

    AABB(const AABB &box0, const AABB &box1) {
        x = Interval(box0.x, box1.x);
        y = Interval(box0.y, box1.y);
        z = Interval(box0.z, box1.z);
    }

    [[nodiscard]] const Interval &axis_interval(int n) const {
        if (n == 1) return y;
        if (n == 2) return z;
        return x;
    }

    [[nodiscard]] bool hit(const Ray &r, Interval ray_t) const {
        const Point3 &ray_orig = r.origin();
        const Vec3 &ray_dir = r.direction();

        for (int axis = 0; axis < 3; axis++) {
            const Interval &ax = axis_interval(axis);
            const double adinv = 1.0 / ray_dir[axis];

            const auto t0 = (ax.min - ray_orig[axis]) * adinv;
            auto t1 = (ax.max - ray_orig[axis]) * adinv;

            if (t0 < t1) {
                if (t0 > ray_t.min) ray_t.min = t0;
                if (t1 < ray_t.max) ray_t.max = t1;
            } else {
                if (t1 > ray_t.min) ray_t.min = t1;
                if (t0 < ray_t.max) ray_t.max = t0;
            }

            if (ray_t.max <= ray_t.min)
                return false;
        }
        return true;
    }

    /**
     * Returns the index of the longest axis of the bounding box.
     * @return index of the longest axis of the bounding box.
     */
    [[nodiscard]] int longest_axis() const {
        if (x.size() > y.size())
            return x.size() > z.size() ? 0 : 2;
        else
            return y.size() > z.size() ? 1 : 2;
    }

    static const AABB empty, universe;

private:

    void pad_to_minimums() {
        // Adjust the AABB so that no side is narrower than some delta, padding if necessary.

        const double delta = 0.0001;
        if (x.size() < delta) x = x.expand(delta);
        if (y.size() < delta) y = y.expand(delta);
        if (z.size() < delta) z = z.expand(delta);
    }
};

AABB operator+(const AABB &bbox, const Vec3 &offset);

AABB operator+(const Vec3 &offset, const AABB &bbox);

#endif //YAPT_AABB_H
