//
// Created by franck on 10/06/24.
//
#include "yapt.h"
#include "aabb.h"

const AABB AABB::empty = AABB(Interval::empty, Interval::empty, Interval::empty);
const AABB AABB::universe = AABB(Interval::universe, Interval::universe, Interval::universe);

AABB operator+(const AABB &bbox, const Vec3 &offset) {
    return AABB(bbox.x + offset.x(), bbox.y + offset.y(), bbox.z + offset.z());
}

AABB operator+(const Vec3 &offset, const AABB &bbox) {
    return bbox + offset;
}