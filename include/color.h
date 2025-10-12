//
// Created by franck on 07/06/24.
//

#ifndef YAPT_COLOR_H
#define YAPT_COLOR_H

#include "Vec3.h"
#include <iostream>

using Color = Vec3;

void writeColor(std::ostream &out, const Color &pixel_color);

double luminance(const Color &pixel_color);

inline double linear_to_gamma(const double linear_component)
{
    if (linear_component > 0)
        return sqrt(linear_component);

    return 0;
}

#endif // YAPT_COLOR_H
