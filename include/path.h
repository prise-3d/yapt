//
// Created by franck on 04/11/24.
//

#ifndef YAPT_PATH_H
#define YAPT_PATH_H

#include "yapt.h"
#include "hittable.h"

class Path {
public:
    Point3 start;
    std::vector<HitRecord> records;
    // current depth
    std::size_t depth;

    Path(Vec3 start, std::size_t max_depth) : start(start), records(std::vector<HitRecord>(max_depth)), depth(0) {}
    ~Path() = default;

    void append(const HitRecord& record) {
        records[depth] = record;
        ++depth;
    }
};
#endif //YAPT_PATH_H
