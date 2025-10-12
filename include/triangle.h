//
// Created by franck on 19/06/24.
//

#ifndef YAPT_TRIANGLE_H
#define YAPT_TRIANGLE_H

#include "yapt.h"
#include "hittable.h"

class Triangle: public Hittable {
public:
    shared_ptr<Material> mat;
    Point3 v[3];
    Vec3 i, j, n;

    Triangle(Point3 a, Vec3 ab, Vec3 ac, shared_ptr<Material> mat): mat(mat), i(ab), j(ac) {
        n = cross(i, j);
        area = n.length();
        n /= area; area /= 2;
        v[0] = a;
        v[1] = a +i ;
        v[2] = a + j;

        double minX, minY, minZ, maxX, maxY, maxZ;
        minX = a.x();
        minY = a.y();
        minZ = a.z();
        maxX = a.x();
        maxY = a.y();
        maxZ = a.z();

        for (int i = 1 ; i < 3 ; i++) {
            Point3 p = v[i];
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

        auto distance_squared = rec.t * rec.t * direction.length2();
        auto cosine = fabs(dot(direction, rec.normal) / direction.length());

        return distance_squared / (cosine * area);
    }

    [[nodiscard]] Vec3 random(const Point3 &origin) const override {
        auto r = random_double();
        auto s = random_double();
        auto t = random_double();
        Point3 inside = (r * v[0] + s * v[1] + t * v[2]) / (r + s + t);
        return inside - origin;
    }

    bool hit(const Ray &r, Interval ray_t, HitRecord &rec) const override {
        Vec3 rayCrossJ = cross(r.direction(), j);
        double det = dot(i, rayCrossJ);

        // r is parallel to the triangle plane
        if (det > -EPSILON && det < EPSILON) return false;

        double invDet = 1. / det;
        Vec3 s = r.origin() - v[0];
        double u = invDet * dot(s, rayCrossJ);

        if (u < 0 || u > 1) return false;

        Vec3 sCrossI = cross(s, i);
        double v = invDet * dot(r.direction(), sCrossI);

        if (v < 0 || u + v > 1) return false;

        // the ray and the triangle intersect

        rec.t = invDet * dot(j, sCrossI);

        if (!(ray_t.contains(rec.t))) return false;

        rec.normal = n;

        rec.mat = mat->get();

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
