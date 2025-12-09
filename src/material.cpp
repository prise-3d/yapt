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

bool Metal::scatter(const Ray &r_in, const HitRecord &rec, ScatterRecord &scatterRecord) const {
    Vec3 reflected = reflect(r_in.direction(), rec.normal);
    reflected = unit_vector(reflected) + (fuzz * random_unit_vector());

    scatterRecord.attenuation = albedo;
    scatterRecord.pdf_ptr = nullptr;
    scatterRecord.skip_pdf = true;
    scatterRecord.skip_pdf_ray = Ray(rec.p, reflected);

    return true;
}

bool Dielectric::scatter(const Ray &r_in, const HitRecord &rec, ScatterRecord &scatterRecord) const {
    scatterRecord.attenuation = Color(1.0, 1.0, 1.0);
    scatterRecord.pdf_ptr = nullptr;
    scatterRecord.skip_pdf = true;
    double ri = rec.front_face ? (1.0 / refraction_index) : refraction_index;

    Vec3 unit_direction = unit_vector(r_in.direction());
    double cos_theta = fmin(dot(-unit_direction, rec.normal), 1.0);
    double sin_theta = sqrt(1.0 - cos_theta * cos_theta);

    bool cannot_refract = ri * sin_theta > 1.0;
    Vec3 direction;

    if (cannot_refract || reflectance(cos_theta, ri) > random_double())
        direction = reflect(unit_direction, rec.normal);
    else
        direction = refract(unit_direction, rec.normal, ri);

    scatterRecord.skip_pdf_ray = Ray(rec.p, direction);
    return true;
}

double Dielectric::reflectance(double cosine, double refraction_index) {
    // Use Schlick's approximation for reflectance.
    auto r0 = (1 - refraction_index) / (1 + refraction_index);
    r0 = r0 * r0;
    return r0 + (1 - r0) * pow((1 - cosine), 5);
}

bool Isotropic::scatter(const Ray &r_in, const HitRecord &rec, ScatterRecord &scatterRecord) const {
    scatterRecord.attenuation = tex->value(rec.u, rec.v, rec.p);
    scatterRecord.pdf_ptr = make_shared<sphere_pdf>();
    scatterRecord.skip_pdf = false;
    return true;
}

double Isotropic::scattering_pdf(const Ray &r_in, const HitRecord &rec, const Ray &scattered) const {
    return 1 / (4 * pi);
}