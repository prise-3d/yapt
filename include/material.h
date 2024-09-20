//
// Created by franck on 09/06/24.
//

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
        return Color(0, 0, 0);
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
    Lambertian(const Color &albedo) : tex(make_shared<SolidColor>(albedo)) {}

    Lambertian(shared_ptr<Texture> tex) : tex(tex) {}

    bool scatter(const Ray &r_in, const HitRecord &rec, ScatterRecord &srec) const override;


    double scattering_pdf(const Ray &r_in, const HitRecord &rec, const Ray &scattered) const override;

private:
    shared_ptr<Texture> tex;
};

class Metal : public Material {
public:
    Metal(const Color &albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const Ray &r_in, const HitRecord &rec, ScatterRecord &srec) const override;

private:
    Color albedo;
    double fuzz;
};

class Dielectric : public Material {
public:
    Dielectric(double refraction_index) : refraction_index(refraction_index) {}

    bool scatter(const Ray &r_in, const HitRecord &rec, ScatterRecord &srec) const override;

private:
    static double reflectance(double cosine, double refraction_index);

    // Refractive index in vacuum or air, or the ratio of the material's refractive index over
    // the refractive index of the enclosing media
    double refraction_index;
};

class DiffuseLight : public Material {
public:
    DiffuseLight(shared_ptr<Texture> tex) : tex(tex) {}

    DiffuseLight(const Color &emit) : tex(make_shared<SolidColor>(emit)) {}

    Color emitted(const Ray &r_in, const HitRecord &rec, double u, double v, const Point3 &p) const override {
        if (!rec.front_face)
            return Color(0, 0, 0);
        return tex->value(u, v, p);
    }

private:
    shared_ptr<Texture> tex;
};

class Isotropic : public Material {
public:
    Isotropic(const Color &albedo) : tex(make_shared<SolidColor>(albedo)) {}

    Isotropic(shared_ptr<Texture> tex) : tex(tex) {}

    bool scatter(const Ray &r_in, const HitRecord &rec, ScatterRecord &srec) const override;

    double scattering_pdf(const Ray &r_in, const HitRecord &rec, const Ray &scattered) const override;

private:
    shared_ptr<Texture> tex;
};


#endif //YAPT_MATERIAL_H
