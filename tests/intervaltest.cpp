//
// Created by franck on 09/06/24.
//

#include "doctest.h"
#include "yapt.h"

TEST_CASE("interval") {
    auto minbound = 0.;
    auto maxbound = 5.;
    auto i = interval(minbound, maxbound);
    CHECK(i.min == minbound);
    CHECK(i.max == maxbound);
    CHECK(i.size() == maxbound - minbound);
    CHECK(i.contains(minbound));
    CHECK(i.contains(maxbound));
    CHECK(i.surrounds(minbound) == false);
    CHECK(i.surrounds(maxbound) == false);
    CHECK(i.surrounds(minbound + EPSILON));
    CHECK(i.surrounds(maxbound - EPSILON));
}

TEST_CASE("interval clamp") {
    auto minbound = 0.;
    auto maxbound = 5.;
    auto midpoint = (minbound + maxbound) / 2;
    auto i = interval(minbound, maxbound);

    CHECK(i.clamp(midpoint) == midpoint);
    CHECK(i.clamp(minbound) == minbound);
    CHECK(i.clamp(maxbound) == maxbound);
    CHECK(i.clamp(minbound - EPSILON) == minbound);
    CHECK(i.clamp(maxbound + EPSILON) == maxbound);
}