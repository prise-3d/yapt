//
// Created by franck on 17/06/24.
//

#ifndef YAPT_PDF_H
#define YAPT_PDF_H

#include "yapt.h"
#include "hittable_list.h"
#include "onb.h"


#include "hittable_list.h"
#include "onb.h"

class PDF {
public:
    virtual ~PDF() = default;

    [[nodiscard]] virtual double value(const Vec3 &direction) const = 0;

    [[nodiscard]] virtual Vec3 generate() const = 0;
};


class cosine_pdf : public PDF {
public:
    explicit cosine_pdf(const Vec3 &w) { uvw.build_from_w(w); }

    [[nodiscard]] double value(const Vec3 &direction) const override {
        const auto cosine_theta = dot(unit_vector(direction), uvw.w());
        return fmax(0, cosine_theta / pi);
    }

    [[nodiscard]] Vec3 generate() const override {
        return uvw.local(random_cosine_direction());
    }

private:
    ONB uvw;
};


class sphere_pdf : public PDF {
public:
    sphere_pdf() = default;

    [[nodiscard]] double value(const Vec3 &direction) const override {
        return 1 / (4 * pi);
    }

    [[nodiscard]] Vec3 generate() const override {
        return random_unit_vector();
    }
};


class HittablePDF : public PDF {
public:
    HittablePDF(const Hittable &objects, const Point3 &origin)
            : objects(objects), origin(origin) {}

    [[nodiscard]] double value(const Vec3 &direction) const override {
        return objects.pdfValue(origin, direction);
    }

    [[nodiscard]] Vec3 generate() const override {
        return objects.random(origin);
    }

private:
    const Hittable &objects;
    Point3 origin;
};


class MixturePDF : public PDF {
public:
    MixturePDF(shared_ptr<PDF> p0, shared_ptr<PDF> p1) {
        p[0] = p0;
        p[1] = p1;
    }

    [[nodiscard]] double value(const Vec3 &direction) const override {
        return 0.5 * p[0]->value(direction) + 0.5 * p[1]->value(direction);
    }

    [[nodiscard]] Vec3 generate() const override {
        if (random_double() < 0.5)
            return p[0]->generate();
        else
            return p[1]->generate();
    }

private:
    shared_ptr<PDF> p[2];
};

#endif //YAPT_PDF_H
