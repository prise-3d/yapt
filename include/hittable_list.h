//
// Created by franck on 09/06/24.
//

#ifndef YAPT_HITTABLE_LIST_H
#define YAPT_HITTABLE_LIST_H

#include "hittable.h"
#include "yapt.h"
#include <vector>

class hittable_list: public hittable {
public:
    std::vector<shared_ptr<hittable>> objects;

    hittable_list();

    hittable_list(const shared_ptr<hittable>& object);

    void clear();

    void add(const shared_ptr<hittable>& object);

    bool hit(const ray &r, double tmin, double tmax, hit_record &record) const override;
};

#endif //YAPT_HITTABLE_LIST_H
