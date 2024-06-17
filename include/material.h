//
// Created by franck on 09/06/24.
//

#ifndef YAPT_MATERIAL_H
#define YAPT_MATERIAL_H

#include "yapt.h"
#include "texture.h"

class hit_record;

class material {
public:
    virtual ~material() = default;

    virtual color emitted(double u, double v, const point3 &p) const {
        return color(0, 0, 0);
    }

    virtual bool scatter(
            const ray &r_in, const hit_record &rec, color &attenuation, ray &scattered
    ) const {
        return false;
    }

    virtual double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered)
    const {
        return 0;
    }
};

class lambertian : public material {
public:
    lambertian(const color &albedo) : tex(make_shared<solid_color>(albedo)) {}

    lambertian(shared_ptr<texture> tex) : tex(tex) {}

    bool scatter(const ray &r_in, const hit_record &rec, color &attenuation, ray &scattered)
    const override;

    double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const;

private:
    shared_ptr<texture> tex;
};

class metal : public material {
public:
    metal(const color &albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const ray &r_in, const hit_record &rec, color &attenuation, ray &scattered)
    const override;

private:
    color albedo;
    double fuzz;
};

class dielectric : public material {
public:
    dielectric(double refraction_index) : refraction_index(refraction_index) {}

    bool scatter(const ray &r_in, const hit_record &rec, color &attenuation, ray &scattered)
    const override;

private:
    static double reflectance(double cosine, double refraction_index);

    // Refractive index in vacuum or air, or the ratio of the material's refractive index over
    // the refractive index of the enclosing media
    double refraction_index;
};

class diffuse_light : public material {
public:
    diffuse_light(shared_ptr<texture> tex) : tex(tex) {}

    diffuse_light(const color &emit) : tex(make_shared<solid_color>(emit)) {}

    color emitted(double u, double v, const point3 &p) const override {
        return tex->value(u, v, p);
    }

private:
    shared_ptr<texture> tex;
};

class isotropic : public material {
public:
    isotropic(const color &albedo) : tex(make_shared<solid_color>(albedo)) {}

    isotropic(shared_ptr<texture> tex) : tex(tex) {}

    bool scatter(const ray &r_in, const hit_record &rec, color &attenuation, ray &scattered)
    const override;

private:
    shared_ptr<texture> tex;
};


#endif //YAPT_MATERIAL_H
