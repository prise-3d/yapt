//
// Created by franck on 04/11/24.
//

#ifndef YAPT_PATH_H
#define YAPT_PATH_H

#include "yapt.h"
#include "hittable.h"

class Path {
public:


    Path(const Vec3& start, const std::size_t max_depth) : start(start), records(std::vector<HitRecord>(max_depth)), depth(0) {}
    ~Path() = default;

    void append(const HitRecord& record) {
        records[depth] = record;
        ++depth;
    }

    void concatenate(const Path& path, const bool reversed) {
        if (reversed) {
            for (std::size_t i = 0; i < path.depth; ++i) {
                append(path.records[depth - i - 1]);
            }
        } else {
            for (std::size_t i = 0; i < path.depth; ++i) {
                append(path.records[i]);
            }
        }
    }

    [[nodiscard]] size_t max_depth() const {
        return records.size();
    }

    [[nodiscard]] Vec3 origin() const { return start; }
    [[nodiscard]] Vec3 destination() const { return records[depth - 1].p; }
    [[nodiscard]] size_t current_depth() const { return depth; }

private:
    Point3 start;
    std::vector<HitRecord> records;
    // current depth
    std::size_t depth;
};


class PathGuidingStrategy {
    public:
    virtual ~PathGuidingStrategy() = default;

    PathGuidingStrategy(shared_ptr<Hittable> scene, shared_ptr<Hittable> lights, size_t max_depth): scene(scene), lights(lights), max_depth(max_depth) {};

    virtual bool grow(Path& start, Path& end);

    [[nodiscard]] bool visible(const Vec3& p, const Vec3& q) const {

        HitRecord record;

        const Ray r(p, q-p);

        return scene->hit(r, Interval(0.0001, 1), record);
    }

    shared_ptr<Hittable> scene;
    shared_ptr<Hittable> lights;
    size_t max_depth;
};

class SimpleGuidingStrategy: public PathGuidingStrategy {
    public:

    SimpleGuidingStrategy(shared_ptr<Hittable> scene, shared_ptr<Hittable> lights, size_t max_depth): PathGuidingStrategy(scene, lights, max_depth) {};

    bool grow(Path& start, Path& end) override {
        //TODO: do something dumb here
        return false;
    }
};


class PathConnector {
public:
    virtual ~PathConnector() = default;

    explicit PathConnector(size_t max_depth): max_depth(max_depth) {};

    // tries to connect start with end
    // in case of failure, relies on strategy to
    // grow the paths
    virtual bool connect(Path& start, Path& end, PathGuidingStrategy* strategy) {
        while (start.current_depth() + end.current_depth() < start.max_depth()) {
            if (strategy->visible(start.destination(), end.destination())) {
                start.concatenate(end, true);
                return true;
            } else {
                strategy->grow(start, end);
            }
        }
        return false;
    }

    size_t max_depth;
};

#endif //YAPT_PATH_H
