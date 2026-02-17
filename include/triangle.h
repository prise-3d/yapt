/*
* This file is part of the YAPT distribution (https://github.com/prise-3d/yapt).
 * Copyright (c) 2025 PrISE-3D.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * --- ADDITIONAL PERMISSION UNDER GNU GPL VERSION 3 SECTION 7 ---
 *
 * If you modify this Program, or any covered work, by linking or
 * combining it with the Intel Math Kernel Library (MKL) (or a modified
 * version of that library), containing parts covered by the terms of
 * the Intel Simplified Software License, the licensors of this
 * Program grant you additional permission to convey the resulting work.
 */

#ifndef YAPT_TRIANGLE_H
#define YAPT_TRIANGLE_H

#include "yapt.h"
#include "hittable.h"

class Triangle: public Hittable {
public:
    shared_ptr<Material> mat;
    Point3 v[3];
    Vec3 i, j, n;

    Triangle(const Point3 &a, const Vec3 &ab, const Vec3 &ac, const std::shared_ptr<Material> &mat): mat(mat), i(ab), j(ac) {
        n = cross(i, j);
        area = n.length();
        n /= area; area /= 2;
        v[0] = a;
        v[1] = a + i ;
        v[2] = a + j;

        double minX = a.x();
        double minY = a.y();
        double minZ = a.z();
        double maxX = a.x();
        double maxY = a.y();
        double maxZ = a.z();

        for (std::size_t k = 1 ; k < 3 ; k++) {
            Point3 p = v[k];
            minX = minX < p.x() ? minX: p.x();
            minY = minY < p.y() ? minY: p.y();
            minZ = minZ < p.z() ? minZ: p.z();
            maxX = maxX < p.x() ? p.x(): maxX;
            maxY = maxY < p.y() ? p.y(): maxY;
            maxZ = maxZ < p.z() ? p.z(): maxZ;
        }

        if (minX == maxX) {
            minX -= EPSILON;
            maxX += EPSILON;
        }
        if (minY == maxY) {
            minY -= EPSILON;
            maxY += EPSILON;
        }
        if (minZ == maxZ) {
            minZ -= EPSILON;
            maxZ += EPSILON;
        }

        bbox = AABB(Point3(minX, minY, minZ), Point3(maxX, maxY, maxZ));
    }

    [[nodiscard]] AABB bounding_box() const override {
        return bbox;
    }

    [[nodiscard]] double pdfValue(const Point3 &origin, const Vec3 &direction) const override {
        HitRecord rec;
        if (!this->hit(Ray(origin, direction), Interval(0.001, infinity), rec))
            return 0;

        const auto distance_squared = rec.t * rec.t * direction.length2();
        const auto cosine = fabs(dot(direction, rec.normal) / direction.length());

        return distance_squared / (cosine * area);
    }

    [[nodiscard]] Vec3 random(const Point3 &origin) const override {
        const auto r = random_double();
        const auto s = random_double();
        const auto t = random_double();
        const Point3 inside = (r * v[0] + s * v[1] + t * v[2]) / (r + s + t);
        return inside - origin;
    }

    bool hit(const Ray &r, Interval ray_t, HitRecord &rec) const override {
        const Vec3 rayCrossJ = cross(r.direction(), j);
        const double det = dot(i, rayCrossJ);

        // r is parallel to the triangle plane
        if (det > -EPSILON && det < EPSILON) return false;

        const double invDet = 1. / det;
        const Vec3 s = r.origin() - v[0];
        const double u = invDet * dot(s, rayCrossJ);

        if (u < 0 || u > 1) return false;

        const Vec3 sCrossI = cross(s, i);
        const double w = invDet * dot(r.direction(), sCrossI);

        if (w < 0 || u + w > 1) return false;

        // the ray and the triangle intersect

        rec.t = invDet * dot(j, sCrossI);

        if (!(ray_t.contains(rec.t))) return false;

        rec.normal = n;

        rec.mat = mat;

        rec.p = r.at(rec.t);
        rec.set_face_normal(r, n);
        return true;
    }

private:
    // vertices
    double area;
    AABB bbox;
};

#endif //YAPT_TRIANGLE_H
