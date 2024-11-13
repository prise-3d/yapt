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

class PathGuidingStrategy {
    public:
    virtual ~PathGuidingStrategy() = default;

    PathGuidingStrategy(shared_ptr<Hittable> scene, shared_ptr<Hittable> lights, size_t max_depth): scene(scene), lights(lights), max_depth(max_depth) {};

    virtual void grow(shared_ptr<Path> start, shared_ptr<Path> end) ;

    shared_ptr<Hittable> scene;
    shared_ptr<Hittable> lights;
    size_t max_depth;
};

class SimpleGuidingStrategy: public PathGuidingStrategy {
    public:

    ~SimpleGuidingStrategy() = default;

    SimpleGuidingStrategy(shared_ptr<Hittable> scene, shared_ptr<Hittable> lights, size_t max_depth): PathGuidingStrategy(scene, lights, max_depth) {};

    void grow(shared_ptr<Path> start, shared_ptr<Path> end) override {
        //TODO: do something dumb here
    }
};


class PathConnector {
public:
    virtual ~PathConnector() = default;

    PathConnector(size_t max_depth): max_depth(max_depth) {};

    // tries to connect start with end
    // in case of failure, relies on strategy to
    virtual shared_ptr<Path> connect(Path& start, Path& end, PathGuidingStrategy* strategy) {
        while (start.depth + end.depth < start.records.size()) {
            /*if (visible (start, end)) {
                return shared_ptr<>(start.concatenate(end.reversed()))
            } else {

                strategy.grow(start, end);
            }*/
        }
        //FIXME: we have a problem
        return nullptr;
    }

    size_t max_depth;

};

#endif //YAPT_PATH_H
