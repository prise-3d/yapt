//
// Created by franck on 07/06/24.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "yapt.h"

TEST_CASE("Vec3 initialization") {
    Vec3 v(1.0, 2.0, 3.0);
    CHECK(v.x() == 1.0);
    CHECK(v.y() == 2.0);
    CHECK(v.z() == 3.0);
}

TEST_CASE("Vec3 member access") {
    Vec3 v(1.0, 2.0, 3.0);
    CHECK(v[0] == 1.0);
    CHECK(v[1] == 2.0);
    CHECK(v[2] == 3.0);
}

TEST_CASE("Vec3 member reference access") {
    Vec3 v(1.0, 2.0, 3.0);
    v[0] += 1;
    v[1] += 1;
    v[2] += 1;
    CHECK(v.x() == 2.0);
    CHECK(v.y() == 3.0);
    CHECK(v.z() == 4.0);
}

TEST_CASE("Vec3 in place addition") {
    Vec3 v1(1.0, 2.0, 3.0);
    Vec3 v2(4.0, 5.0, 6.0);
    v1 += v2;
    CHECK(v1.x() == 5.0);
    CHECK(v1.y() == 7.0);
    CHECK(v1.z() == 9.0);
}

TEST_CASE("Vec3 in place multiplication") {
    Vec3 v1(1.0, 2.0, 3.0);

    v1 *= 2.0;
    CHECK(v1.x() == 2.0);
    CHECK(v1.y() == 4.0);
    CHECK(v1.z() == 6.0);
}

TEST_CASE("Vec3 in place division") {
    Vec3 v1(2.0, 4.0, 6.0);

    v1 /= 2.0;
    CHECK(v1.x() == 1.0);
    CHECK(v1.y() == 2.0);
    CHECK(v1.z() == 3.0);
}

TEST_CASE("Vec3 addition") {
    Vec3 v1(1.0, 2.0, 3.0);
    Vec3 v2(4.0, 5.0, 6.0);
    Vec3 v3 = v1 + v2;
    CHECK(v3.x() == 5.0);
    CHECK(v3.y() == 7.0);
    CHECK(v3.z() == 9.0);
}

TEST_CASE("Vec3 opposite") {
    Vec3 v1(1.0, 2.0, 3.0);
    Vec3 v2 = -v1;
    CHECK((v1 + v2).near_zero());
}

TEST_CASE("Vec3 subtraction") {
    Vec3 v1(1.0, 2.0, 3.0);
    Vec3 v2(2.0, 3.0, 4.0);
    Vec3 v3 = v2 - v1;
    CHECK(v3.x() == 1.0);
    CHECK(v3.y() == 1.0);
    CHECK(v3.z() == 1.0);
}

TEST_CASE("Vec3 multiplication") {
    Vec3 v1(1.0, 2.0, 3.0);
    Vec3 v2 = 2 * v1;
    Vec3 v3 = v1 * 2;

    CHECK(v2.x() == 2.0);
    CHECK(v2.y() == 4.0);
    CHECK(v2.z() == 6.0);
    CHECK(v3.x() == 2.0);
    CHECK(v3.y() == 4.0);
    CHECK(v3.z() == 6.0);
}

TEST_CASE("Vec3 division") {
    Vec3 v1(1.0, 2.0, 3.0);
    Vec3 v2 = v1 / 2;

    CHECK(v2.x() == .5);
    CHECK(v2.y() == 1.0);
    CHECK(v2.z() == 1.5);
}

TEST_CASE("Vec3 length") {
    Vec3 v1(3.0, 2.0, 6.0);
    double l = v1.length();
    CHECK(l == 7.0);
}

TEST_CASE("Vec3 dot product") {
    Vec3 v1(3.0, 2.0, 6.0);
    Vec3 v2(2.0, -2.0, 3.0);
    auto l = dot(v1, v2);
    CHECK(l == 20.0);
}

TEST_CASE("Vec3 unit vector") {
    Vec3 v1 (2., 1.5, 7.);
    Vec3 v2 = unit_vector(v1);

    CHECK(v2.length2() == 1.);
}

TEST_CASE("Vec3 normalization") {
    Vec3 v1(3.0, 2.0, 6.0);
    Vec3 v2 = unit_vector(v1);
    CHECK(v2.x() == 3./7.);
    CHECK(v2.y() == 2./7.);
    CHECK(v2.z() == 6./7.);
}

TEST_CASE("Vec3 cross product") {
    Vec3 i(1, 0, 0);
    Vec3 j(0, 1, 0);
    Vec3 k(0, 0, 1);

    Vec3 ij = cross(i, j);
    Vec3 jk = cross(j, k);
    Vec3 ki = cross(k , i);

    Vec3 ii = cross(i, i);

    CHECK(ij.x() == 0);
    CHECK(ij.y() == 0);
    CHECK(ij.z() == 1);

    CHECK(ki.x() == 0);
    CHECK(ki.y() == 1);
    CHECK(ki.z() == 0);

    CHECK(jk.x() == 1);
    CHECK(jk.y() == 0);
    CHECK(jk.z() == 0);

    CHECK(ii.x() == 0);
    CHECK(ii.y() == 0);
    CHECK(ii.z() == 0);
}