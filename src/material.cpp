//
// Created by franck on 09/06/24.
//

#include "yapt.h"
#include "material.h"
#include "hittable.h"

bool Lambertian::scatter(const Ray &r_in, const HitRecord &rec, ScatterRecord &srec) const {
    srec.attenuation = tex->value(rec.u, rec.v, rec.p);
    srec.pdf_ptr = make_shared<cosine_pdf>(rec.normal);
    srec.skip_pdf = false;
    return true;
}


double Lambertian::scattering_pdf(const Ray &r_in, const HitRecord &rec, const Ray &scattered) const {
    auto cos_theta = dot(rec.normal, unit_vector(scattered.direction()));
    return cos_theta < 0 ? 0 : cos_theta / pi;
}

bool Metal::scatter(const Ray &r_in, const HitRecord &rec, ScatterRecord &srec) const {
    Vec3 reflected = reflect(r_in.direction(), rec.normal);
    reflected = unit_vector(reflected) + (fuzz * random_unit_vector());

    srec.attenuation = albedo;
    srec.pdf_ptr = nullptr;
    srec.skip_pdf = true;
    srec.skip_pdf_ray = Ray(rec.p, reflected);

    return true;
}

bool Dielectric::scatter(const Ray &r_in, const HitRecord &rec, ScatterRecord &srec) const {
    srec.attenuation = Color(1.0, 1.0, 1.0);
    srec.pdf_ptr = nullptr;
    srec.skip_pdf = true;
    double ri = rec.front_face ? (1.0 / refraction_index) : refraction_index;

    Vec3 unit_direction = unit_vector(r_in.direction());
    double cos_theta = fmin(dot(-unit_direction, rec.normal), 1.0);
    double sin_theta = sqrt(1.0 - cos_theta * cos_theta);

    bool cannot_refract = ri * sin_theta > 1.0;
    Vec3 direction;

    if (cannot_refract || reflectance(cos_theta, ri) > randomDouble())
        direction = reflect(unit_direction, rec.normal);
    else
        direction = refract(unit_direction, rec.normal, ri);

    srec.skip_pdf_ray = Ray(rec.p, direction);
    return true;
}

double Dielectric::reflectance(double cosine, double refraction_index) {
    // Use Schlick's approximation for reflectance.
    auto r0 = (1 - refraction_index) / (1 + refraction_index);
    r0 = r0 * r0;
    return r0 + (1 - r0) * pow((1 - cosine), 5);
}

bool Isotropic::scatter(const Ray &r_in, const HitRecord &rec, ScatterRecord &srec) const {
    srec.attenuation = tex->value(rec.u, rec.v, rec.p);
    srec.pdf_ptr = make_shared<sphere_pdf>();
    srec.skip_pdf = false;
    return true;
}

double Isotropic::scattering_pdf(const Ray &r_in, const HitRecord &rec, const Ray &scattered) const {
    return 1 / (4 * pi);
}