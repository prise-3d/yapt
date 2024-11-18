//
// Created by franck on 09/06/24.
//

#ifndef YAPT_HITTABLE_LIST_H
#define YAPT_HITTABLE_LIST_H

#include "yapt.h"
#include "hittable.h"
#include <vector>

class HittableList : public Hittable {
public:
    std::vector<shared_ptr<Hittable>> objects;

    HittableList();

    explicit HittableList(const shared_ptr<Hittable> &object);

    void clear();

    void add(const shared_ptr<Hittable> &object);

    bool hit(const Ray &r, Interval ray_t, HitRecord &record) const override;

    [[nodiscard]] AABB boundingBox() const override;

    [[nodiscard]] double pdfValue(const Point3 &origin, const Vec3 &direction) const override;

    [[nodiscard]] Vec3 random(const Point3 &origin) const override;

private:
    AABB bbox;
};

#endif //YAPT_HITTABLE_LIST_H
