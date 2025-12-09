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
