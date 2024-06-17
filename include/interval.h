//
// Created by franck on 09/06/24.
//

#ifndef YAPT_INTERVAL_H
#define YAPT_INTERVAL_H

#include "constants.h"

class Interval {
public:
    double min, max;

    Interval();

    Interval(double min, double max);

    Interval(const Interval &a, const Interval &b);

    double size() const;

    bool contains(double x) const;

    bool surrounds(double x) const;

    double clamp(double x) const;

    Interval expand(double delta) const;

    static const Interval empty, universe, future;
};

Interval operator+(const Interval &ival, double displacement);

Interval operator+(double displacement, const Interval &ival);

#endif //YAPT_INTERVAL_H
