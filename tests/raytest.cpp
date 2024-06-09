//
// Created by franck on 07/06/24.
//

#include "doctest.h"
#include "vec3.h"
#include "ray.h"

TEST_CASE("default constructor") {
    ray r;

    CHECK(is_null(r.direction()));
    CHECK(is_null(r.origin()));
}

TEST_CASE("ray constructor") {
    vec3 d = vec3(1, 2, 3);
    point3 o = point3(-1, -2, -3);
    ray r(o, d);
    CHECK(are_equal(d, r.direction()));
    CHECK(are_equal(o, r.origin()));
}

TEST_CASE("ray evaluation") {
    vec3 d = vec3(.5, 1, 1.5);
    point3 o = point3(3, 2, 3);
    ray r = ray(o, d);
    point3 p = r.at(2.);
    CHECK(are_equal(p, point3(4, 4, 6)));
}

TEST_CASE("shoot ray") {
    point3 o = point3(1, 2, 3);
    point3 p = point3(-6, 1, 4);
    auto r = ray::shoot(o, p);
    CHECK(are_equal(o, r.origin()));
    CHECK(are_equal(vec3(-7, -1, 1), r.direction()));
}