//
// Created by franck on 17/12/24.
//

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
