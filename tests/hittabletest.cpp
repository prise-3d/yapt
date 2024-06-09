//
// Created by franck on 09/06/24.
//

#include "doctest.h"
#include "vec3.h"
#include "ray.h"
#include "sphere.h"
#include "hittable_list.h"

TEST_CASE("hittable list") {
    auto o1 = point3(0, 0, 0);
    auto o2 = point3(10, 0, 0);
    auto p2 = .5 * (o1 + o2);
    auto radius = 1;
    auto s1 = sphere(o1, radius);
    auto s2 = sphere(o2, radius);

    auto objects = hittable_list(make_shared<sphere>(s1));
    objects.add(make_shared<sphere>(s2));

    auto ray_t = interval::future;

    SUBCASE("no hit") {
        hit_record record;
        auto p1 = point3(12, 12, 12);
        auto r = ray::shoot(p1, p2);

        CHECK(objects.hit(r, ray_t, record) == false);
    }

    SUBCASE("hit s1 from in between") {
        hit_record record;

        auto r = ray::shoot(p2, o1);
        objects.hit(r, ray_t, record);

        CHECK(s1.has(record.p));
    }

    SUBCASE("hit s2 from in between") {
        hit_record record;

        auto r = ray::shoot(p2, o2);
        objects.hit(r, ray_t, record);

        CHECK(s2.has(record.p));
    }

    SUBCASE("hit s1 from away from s2") {
        hit_record record;

        auto orig = o1 + (o1 - o2) + vec3(0, 1, 0);

        auto r = ray::shoot(orig, o2);
        objects.hit(r, ray_t, record);

        CHECK(s1.has(record.p));
    }

    SUBCASE("hit s2 from away from s1") {
        hit_record record;

        auto orig = o2 + (o2 - o1) + vec3 (0, 0, 1);

        auto r = ray::shoot(orig, o1);
        objects.hit(r, ray_t, record);

        CHECK(s2.has(record.p));
    }
}