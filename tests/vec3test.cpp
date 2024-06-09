//
// Created by franck on 07/06/24.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "yapt.h"

TEST_CASE("vec3 initialization") {
    vec3 v(1.0, 2.0, 3.0);
    CHECK(v.x() == 1.0);
    CHECK(v.y() == 2.0);
    CHECK(v.z() == 3.0);
}

TEST_CASE("vec3 member access") {
    vec3 v(1.0, 2.0, 3.0);
    CHECK(v[0] == 1.0);
    CHECK(v[1] == 2.0);
    CHECK(v[2] == 3.0);
}

TEST_CASE("vec3 member reference access") {
    vec3 v(1.0, 2.0, 3.0);
    v[0] += 1;
    v[1] += 1;
    v[2] += 1;
    CHECK(v.x() == 2.0);
    CHECK(v.y() == 3.0);
    CHECK(v.z() == 4.0);
}

TEST_CASE("vec3 in place addition") {
    vec3 v1(1.0, 2.0, 3.0);
    vec3 v2(4.0, 5.0, 6.0);
    v1 += v2;
    CHECK(v1.x() == 5.0);
    CHECK(v1.y() == 7.0);
    CHECK(v1.z() == 9.0);
}

TEST_CASE("vec3 in place multiplication") {
    vec3 v1(1.0, 2.0, 3.0);

    v1 *= 2.0;
    CHECK(v1.x() == 2.0);
    CHECK(v1.y() == 4.0);
    CHECK(v1.z() == 6.0);
}

TEST_CASE("vec3 in place division") {
    vec3 v1(2.0, 4.0, 6.0);

    v1 /= 2.0;
    CHECK(v1.x() == 1.0);
    CHECK(v1.y() == 2.0);
    CHECK(v1.z() == 3.0);
}

TEST_CASE("vec3 addition") {
    vec3 v1(1.0, 2.0, 3.0);
    vec3 v2(4.0, 5.0, 6.0);
    vec3 v3 = v1 + v2;
    CHECK(v3.x() == 5.0);
    CHECK(v3.y() == 7.0);
    CHECK(v3.z() == 9.0);
}

TEST_CASE("vec3 subtraction") {
    vec3 v1(1.0, 2.0, 3.0);
    vec3 v2(2.0, 3.0, 4.0);
    vec3 v3 = v2 - v1;
    CHECK(v3.x() == 1.0);
    CHECK(v3.y() == 1.0);
    CHECK(v3.z() == 1.0);
}

TEST_CASE("vec3 multiplication") {
    vec3 v1(1.0, 2.0, 3.0);
    vec3 v2 = 2 * v1;
    vec3 v3 = v1 * 2;

    CHECK(v2.x() == 2.0);
    CHECK(v2.y() == 4.0);
    CHECK(v2.z() == 6.0);
    CHECK(v3.x() == 2.0);
    CHECK(v3.y() == 4.0);
    CHECK(v3.z() == 6.0);
}

TEST_CASE("vec3 division") {
    vec3 v1(1.0, 2.0, 3.0);
    vec3 v2 = v1 / 2;

    CHECK(v2.x() == .5);
    CHECK(v2.y() == 1.0);
    CHECK(v2.z() == 1.5);
}

TEST_CASE("vec3 length") {
    vec3 v1(3.0, 2.0, 6.0);
    double l = v1.length();
    CHECK(l == 7.0);
}

TEST_CASE("vec3 dot product") {
    vec3 v1(3.0, 2.0, 6.0);
    vec3 v2(2.0, -2.0, 3.0);
    auto l = dot(v1, v2);
    CHECK(l == 20.0);
}

TEST_CASE("vec3 unit vector") {
    vec3 v1 (2., 1.5, 7.);
    vec3 v2 = unit_vector(v1);

    CHECK(v2.length2() == 1.);
}

TEST_CASE("vec3 normalization") {
    vec3 v1(3.0, 2.0, 6.0);
    vec3 v2 = unit_vector(v1);
    CHECK(v2.x() == 3./7.);
    CHECK(v2.y() == 2./7.);
    CHECK(v2.z() == 6./7.);
}

TEST_CASE("vec3 cross product") {
    vec3 i(1, 0, 0);
    vec3 j(0, 1, 0);
    vec3 k(0, 0, 1);

    vec3 ij = cross(i, j);
    vec3 jk = cross(j, k);
    vec3 ki = cross(k ,i);

    vec3 ii = cross(i, i);

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