//
// Created by franck on 04/11/24.
//

#ifndef YAPT_PATH_H
#define YAPT_PATH_H

#include "yapt.h"
#include "hittable.h"
#include "material.h"


// TODO: make PathStep carry more data to account for contributions?
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
    Path(std::size_t max_depth, Ray ray, shared_ptr<Hittable> hittable);

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
