//
// Created by franck on 09/06/24.
//

#include "doctest.h"
#include "Vec3.h"
#include "ray.h"
#include "sphere.h"
#include "hittable_list.h"

TEST_CASE("hittable list") {
    auto o1 = Point3(0, 0, 0);
    auto o2 = Point3(10, 0, 0);
    auto p2 = .5 * (o1 + o2);
    auto radius = 1;
    auto s1 = Sphere(o1, radius);
    auto s2 = Sphere(o2, radius);

    auto objects = HittableList(make_shared<Sphere>(s1));
    objects.add(make_shared<Sphere>(s2));

    auto ray_t = Interval::future;

    SUBCASE("no hit") {
        HitRecord record;
        auto p1 = Point3(12, 12, 12);
        auto r = Ray::shoot(p1, p2);

        CHECK(objects.hit(r, ray_t, record) == false);
    }

    SUBCASE("hit s1 from in between") {
        HitRecord record;

        auto r = Ray::shoot(p2, o1);
        objects.hit(r, ray_t, record);

        CHECK(s1.has(record.p));
    }

    SUBCASE("hit s2 from in between") {
        HitRecord record;

        auto r = Ray::shoot(p2, o2);
        objects.hit(r, ray_t, record);

        CHECK(s2.has(record.p));
    }

    SUBCASE("hit s1 from away from s2") {
        HitRecord record;

        auto orig = o1 + (o1 - o2) + Vec3(0, 1, 0);

        auto r = Ray::shoot(orig, o2);
        objects.hit(r, ray_t, record);

        CHECK(s1.has(record.p));
    }

    SUBCASE("hit s2 from away from s1") {
        HitRecord record;

        auto orig = o2 + (o2 - o1) + Vec3 (0, 0, 1);

        auto r = Ray::shoot(orig, o1);
        objects.hit(r, ray_t, record);

        CHECK(s2.has(record.p));
    }
}