//
// Created by franck on 08/06/24.
//

#include "doctest.h"
#include "Vec3.h"
#include "ray.h"
#include "sphere.h"
#include "yapt.h"

TEST_CASE("SPHERE1") {
    Point3 o(0, 0, 0);
    Point3 p(4, 1, 0.1);
    Sphere s(o, .001);
    Ray r = Ray::shoot(p, o);
    HitRecord record;

    CHECK(s.hit(r, Interval::future, record));
}

TEST_CASE("sphere surface normal") {
    Point3 o(1, 1, 1);
    double radius = 1.2;
    Sphere s(o, radius);

    Point3 a(7, 4, 6);
    auto r1 = Ray::shoot(o, a);
    auto r2 = Ray::shoot(a, o);

    HitRecord record1;
    HitRecord record2;
    auto result1 = s.hit(r1, Interval::future, record1);
    auto result2 = s.hit(r2, Interval::future, record2);
    CHECK(result1);
    CHECK(result2);

    auto p1 = record1.p;
    auto p2 = record2.p;

    CHECK(are_epsilon_equal(p1, p2));
    CHECK(dot(record1.normal, (r1.direction())) < 0);
    CHECK(cross(record1.normal, (r1.direction())).length2() < EPSILON);
    CHECK(dot(record2.normal, (r2.direction())) < 0);
    CHECK(cross(record2.normal, (r2.direction())).length2() < EPSILON);
}
