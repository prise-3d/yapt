/*
 * This file is part of the YAPT distribution (https://github.com/prise-3d/yapt).
 * Copyright (c) 2025 PrISE-3D.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * --- ADDITIONAL PERMISSION UNDER GNU GPL VERSION 3 SECTION 7 ---
 *
 * If you modify this Program, or any covered work, by linking or
 * combining it with the Intel Math Kernel Library (MKL) (or a modified
 * version of that library), containing parts covered by the terms of
 * the Intel Simplified Software License, the licensors of this
 * Program grant you additional permission to convey the resulting work.
 */

#ifndef YAPT_PATH_H
#define YAPT_PATH_H

#include "yapt.h"
#include "hittable.h"
#include "material.h"

class PathStep {
public:
    HitRecord hitRecord;
    ScatterRecord scatterRecord;

    PathStep() = default;
    explicit PathStep(const HitRecord &hitRecord): hitRecord(hitRecord), scatterRecord() {}

    void registerScatterRecord(const ScatterRecord &scatterRecord) {
        this->scatterRecord = scatterRecord;
    }
};

class Path {
public:
    explicit Path() = default;
    explicit Path(std::size_t max_depth);
    Path(std::size_t max_depth, PathStep &step);

    virtual ~Path() = default;

    void append(const PathStep &step);
    [[nodiscard]] PathStep lastStep() const;

    [[nodiscard]] PathStep get(size_t index) const;
    [[nodiscard]] size_t getDepth() const;
    [[nodiscard]] size_t getMaxDepth() const;
    [[nodiscard]] Ray incomingRay() const;
    void registerScatterRecord(const ScatterRecord &scatterRecord);

    std::vector<PathStep> steps;
    size_t depth = 0;
};

class PathGuidingStrategy {
public:
    virtual ~PathGuidingStrategy() = default;

    PathGuidingStrategy(shared_ptr<Hittable> scene, shared_ptr<Hittable> lights, size_t max_depth): scene(scene), lights(lights), max_depth(max_depth) {};

    virtual bool grow(Path& path) = 0;
    [[nodiscard]] bool visible(const Vec3& p, const Vec3& q) const;
    bool connect(Path &cameraPath, const Path &lightPath) const;

    shared_ptr<Hittable> scene;
    shared_ptr<Hittable> lights;
    size_t max_depth;
};

class SimpleGuidingStrategy: public PathGuidingStrategy {
    public:

    SimpleGuidingStrategy(shared_ptr<Hittable> scene, shared_ptr<Hittable> lights, size_t max_depth): PathGuidingStrategy(scene, lights, max_depth) {};

    bool grow(Path& path) override;
};

#endif //YAPT_PATH_H
