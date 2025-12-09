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

#ifndef UTILS_H
#define UTILS_H


inline QColor toQColor(const double r, const double g, const double b) {
    static const Interval intensity(0.000, 0.999);
    const double rr = linear_to_gamma(r);
    const double gg = linear_to_gamma(g);
    const double bb = linear_to_gamma(b);

    const int ir = static_cast<int>(256 * intensity.clamp(rr));
    const int ig = static_cast<int>(256 * intensity.clamp(gg));
    const int ib = static_cast<int>(256 * intensity.clamp(bb));

    return {ir, ig, ib};
}

inline QColor toQColor(const Color &color) {
    return toQColor(color.x(), color.y(), color.z());
}

#endif //UTILS_H
