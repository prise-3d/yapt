//
// Created by franck on 07/06/24.
//

#include "doctest.h"
#include "Vec3.h"
#include "ray.h"

TEST_CASE("default constructor") {
    Ray r;

    CHECK(is_null(r.direction()));
    CHECK(is_null(r.origin()));
}

TEST_CASE("ray constructor") {
    Vec3 d = Vec3(1, 2, 3);
    Point3 o = Point3(-1, -2, -3);
    Ray r(o, d);
    CHECK(are_equal(d, r.direction()));
    CHECK(are_equal(o, r.origin()));
}

TEST_CASE("ray evaluation") {
    Vec3 d = Vec3(.5, 1, 1.5);
    Point3 o = Point3(3, 2, 3);
    Ray r = Ray(o, d);
    Point3 p = r.at(2.);
    CHECK(are_equal(p, Point3(4, 4, 6)));
}

TEST_CASE("shoot ray") {
    Point3 o = Point3(1, 2, 3);
    Point3 p = Point3(-6, 1, 4);
    auto r = Ray::shoot(o, p);
    CHECK(are_equal(o, r.origin()));
    CHECK(are_equal(Vec3(-7, -1, 1), r.direction()));
}