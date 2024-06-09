//
// Created by franck on 08/06/24.
//

#include "doctest.h"
#include "vec3.h"
#include "ray.h"
#include "sphere.h"
#include <iostream>

TEST_CASE("SPHERE1") {
    point3 o(0, 0, 0);
    point3 p(4, 1, 0.1);
    sphere s(o, .001);
    ray r = ray::shoot(p, o);
    hit_record record;

    CHECK(s.hit(r, 0, 9999, record));
}

TEST_CASE("sphere surface normal") {
    point3 o(1, 1, 1);
    double radius = 1.2;
    sphere s(o, radius);

    point3 a(7,4,6);
    auto r1 = ray::shoot(o, a);
    auto r2 = ray::shoot(a, o);

    hit_record record1;
    hit_record record2;
    auto result1 = s.hit(r1, 0, 9999, record1);
    auto result2 = s.hit(r2, 0, 9999, record2);
    CHECK(result1);
    CHECK(result2);

    auto p1 = record1.p;
    auto p2 = record2.p;

    CHECK(are_epsilon_equal(p1, p2));
    CHECK(dot(record1.normal, (r1.direction())) < 0);
    CHECK(cross(record1.normal, (r1.direction())).length2() < vec3::EPSILON);
    CHECK(dot(record2.normal, (r2.direction())) < 0);
    CHECK(cross(record2.normal, (r2.direction())).length2() < vec3::EPSILON);
}
