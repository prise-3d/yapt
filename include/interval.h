//
// Created by franck on 09/06/24.
//

#ifndef YAPT_INTERVAL_H
#define YAPT_INTERVAL_H

#include "constants.h"

class interval {
public:
    double min, max;

    interval();

    interval(double min, double max);

    double size() const;

    bool contains(double x) const;

    bool surrounds(double x) const;

    double clamp(double x) const;

    static const interval empty, universe, future;
};

#endif //YAPT_INTERVAL_H
