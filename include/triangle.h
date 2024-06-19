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
    }

    [[nodiscard]] AABB boundingBox() const override {
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
        auto r = randomDouble();
        auto s = randomDouble();
        auto t = randomDouble();
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
        rec.mat = mat;
        rec.set_face_normal(r, n);
        rec.u = u;
        rec.v = v;
        return true;
    }

private:
    // vertices
    double area;
    AABB bbox;

};



#endif //YAPT_TRIANGLE_H
