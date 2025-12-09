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

#ifndef YAPT_MATERIAL_H
#define YAPT_MATERIAL_H

#include "yapt.h"
#include "pdf.h"
#include "texture.h"

class HitRecord;

class ScatterRecord {
public:
    Color attenuation;
    shared_ptr<PDF> pdf_ptr;
    bool skip_pdf;
    Ray skip_pdf_ray;
};

class Material {
public:
    virtual ~Material() = default;

    virtual Color emitted(
            const Ray &r_in, const HitRecord &rec, double u, double v, const Point3 &p
    ) const {
        return {0, 0, 0};
    }

    virtual bool scatter(const Ray &r_in, const HitRecord &rec, ScatterRecord &srec) const {
        return false;
    }

    virtual double scattering_pdf(const Ray &r_in, const HitRecord &rec, const Ray &scattered)
    const {
        return 0;
    }
};

class Lambertian : public Material {
public:
    explicit Lambertian(const Color &albedo) : tex(make_shared<SolidColor>(albedo)) {}

    explicit Lambertian(shared_ptr<Texture> tex) : tex(tex) {}

    bool scatter(const Ray &r_in, const HitRecord &rec, ScatterRecord &scatterRecord) const override;

    double scattering_pdf(const Ray &r_in, const HitRecord &rec, const Ray &scattered) const override;

private:
    shared_ptr<Texture> tex;
};

class Metal : public Material {
public:
    Metal(const Color &albedo, const double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const Ray &r_in, const HitRecord &rec, ScatterRecord &scatterRecord) const override;

private:
    Color albedo;
    double fuzz;
};

class Dielectric : public Material {
public:
    explicit Dielectric(const double refraction_index) : refraction_index(refraction_index) {}

    bool scatter(const Ray &r_in, const HitRecord &rec, ScatterRecord &scatterRecord) const override;

private:
    static double reflectance(double cosine, double refraction_index);

    // Refractive index in vacuum or air, or the ratio of the material's refractive index over
    // the refractive index of the enclosing media
    double refraction_index;
};

class DiffuseLight : public Material {
public:
    explicit DiffuseLight(shared_ptr<Texture> tex) : tex(tex) {}

    explicit DiffuseLight(const Color &emit) : tex(make_shared<SolidColor>(emit)) {}

    Color emitted(const Ray &r_in, const HitRecord &rec, double u, double v, const Point3 &p) const override {
        if (!rec.front_face)
            return {0, 0, 0};
        return tex->value(u, v, p);
    }

private:
    shared_ptr<Texture> tex;
};

class Isotropic : public Material {
public:
    explicit Isotropic(const Color &albedo) : tex(make_shared<SolidColor>(albedo)) {}

    explicit Isotropic(shared_ptr<Texture> tex) : tex(tex) {}

    bool scatter(const Ray &r_in, const HitRecord &rec, ScatterRecord &scatterRecord) const override;

    double scattering_pdf(const Ray &r_in, const HitRecord &rec, const Ray &scattered) const override;

private:
    shared_ptr<Texture> tex;
};

#endif //YAPT_MATERIAL_H
