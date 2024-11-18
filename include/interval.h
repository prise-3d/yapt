//
// Created by franck on 09/06/24.
//

#ifndef YAPT_INTERVAL_H
#define YAPT_INTERVAL_H

class Interval {
public:
    double min, max;

    Interval();

    Interval(double min, double max);

    Interval(const Interval &a, const Interval &b);

    [[nodiscard]] double size() const;

    [[nodiscard]] bool contains(double x) const;

    [[nodiscard]] bool surrounds(double x) const;

    [[nodiscard]] double clamp(double x) const;

    [[nodiscard]] Interval expand(double delta) const;

    static const Interval empty, universe, future;
};

Interval operator+(const Interval &interval, double displacement);

Interval operator+(double displacement, const Interval &interval);

#endif //YAPT_INTERVAL_H
