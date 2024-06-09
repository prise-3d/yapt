//
// Created by franck on 08/06/24.
//

#include "doctest.h"
#include "vec3.h"
#include "ray.h"
#include "sphere.h"

TEST_CASE("SPHERE1") {
    point3 o(0, 0, 0);
    point3 p(4, 1, 0.1);
    sphere s(o, .001);
    ray r = shoot_ray(p, o);
    hit_record record;

    CHECK(s.hit(r, 0, 9999, record));
}
