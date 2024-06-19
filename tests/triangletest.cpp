//
// Created by franck on 19/06/24.
//

#include "doctest.h"
#include "yapt.h"
#include "triangle.h"
#include "material.h"

TEST_CASE("hit in triangle") {
    // hit vertex

    auto l = make_shared<Lambertian>(nullptr);
    for (int h = 0 ; h < 100 ; h++) {
        Triangle t(
                {randomDouble(), randomDouble(), randomDouble()},
                {randomDouble(), randomDouble(), randomDouble()},
                {randomDouble(), randomDouble(), randomDouble()},
                l
        );

        //Triangle t({0, 0, 0}, {1, 0, 0}, {0, 1, 0});
        Point3 origin(-10, 0, 0);
        HitRecord rec;

        // These MIGHT fail for numerical reasons
        // kept here for reference
        /*CHECK(t.hit(Ray(origin, t.v[0] - origin), Interval::future, rec));
        CHECK(t.hit(Ray(origin, t.v[1] - origin), Interval::future, rec));
        CHECK(t.hit(Ray(origin, t.v[2] - origin), Interval::future, rec));*/

        // miss
        double x = randomDouble();
        double y = randomDouble();
        double z = randomDouble();

        Point3 a = (-x * t.v[0] + y * t.v[1] + z * t.v[2]) / (-x + y + z);
        Point3 b = (x * t.v[0] - y * t.v[1] + z * t.v[2]) / (x - y + z);
        Point3 c = (x * t.v[0] + y * t.v[1] - z * t.v[2]) / (x + y - z);

        CHECK(t.hit(Ray(origin, a - origin), Interval::future, rec) == false);
        CHECK(t.hit(Ray(origin, b - origin), Interval::future, rec) == false);
        CHECK(t.hit(Ray(origin, c - origin), Interval::future, rec) == false);

        for (int i = 0; i < 10; i++) {
            x = randomDouble();
            y = randomDouble();
            z = randomDouble();

            // inside the triangle
            a = (x * t.v[0] + y * t.v[1] + z * t.v[2]) / (x + y + z);

            CHECK(t.hit(Ray(origin, a - origin), Interval::future, rec));
        }
    }
}
