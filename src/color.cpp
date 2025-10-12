//
// Created by franck on 09/06/24.
//

#include "yapt.h"

void writeColor(std::ostream &out, const Color &pixel_color) {
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    // Apply a linear to gamma transform for gamma 2
    r = linear_to_gamma(r);
    g = linear_to_gamma(g);
    b = linear_to_gamma(b);

    // Translate the [0,1] component values to the byte range [0,255].
    static const Interval intensity(0.000, 0.999);
    int rbyte = int(256 * intensity.clamp(r));
    int gbyte = int(256 * intensity.clamp(g));
    int bbyte = int(256 * intensity.clamp(b));

    // Write out the pixel color components.
    out << rbyte << ' ' << gbyte << ' ' << bbyte << '\n';
}

/**
 * compute luminance according the ITU-R BT.709 standard
 */
double luminance(const Color &pixel_color)
{
    return 0.2126 * pixel_color.x() + 0.7152 * pixel_color.y() + 0.0722 * pixel_color.z();
}