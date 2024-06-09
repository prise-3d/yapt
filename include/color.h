//
// Created by franck on 07/06/24.
//

#ifndef YAPT_COLOR_H
#define YAPT_COLOR_H

#include "vec3.h"
#include <iostream>

using color = vec3;

void write_color(std::ostream &out, const color &pixel_color);

inline double linear_to_gamma(double linear_component)
{
    if (linear_component > 0)
        return sqrt(linear_component);

    return 0;
}

#endif //YAPT_COLOR_H
