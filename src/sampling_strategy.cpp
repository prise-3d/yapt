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

#include "sampling_strategy.h"
#include "pdf.h"

Color NEESamplingStrategy::compute_scattered_color(
    const SamplingContext& context,
    const std::function<Color(const Ray&, int)>& ray_color_function
) const {
    Color colorFromScatter{0, 0, 0};

    // Next Event Estimation: Sample a point on the light
    auto light_ptr = make_shared<HittablePDF>(context.lights, context.hit_record.p);
    Ray light_ray(context.hit_record.p, light_ptr->generate());
    double light_pdf = light_ptr->value(light_ray.direction());

    if (light_pdf > 0) {
        HitRecord light_rec;
        // Check if the light is visible or oocluded
        context.world.hit(light_ray, Interval(0.001, INFINITY), light_rec);
        if (light_rec.t > 0.9999) {
            Color light_emission = light_rec.mat->emitted(light_ray, light_rec,
                                                          light_rec.u, light_rec.v, light_rec.p);
            if (light_emission.length2() > 0) {
                double scattering_pdf = context.hit_record.mat->scattering_pdf(
                    context.incoming_ray, context.hit_record, light_ray);

                // POWER HEURISTIC (beta = 2)
                double light_pdf_2 = light_pdf * light_pdf;
                double scattering_pdf_2 = scattering_pdf * scattering_pdf;
                double weight_nee = light_pdf_2 / (light_pdf_2 + scattering_pdf_2);

                colorFromScatter += weight_nee * context.scatter_record.attenuation *
                                   scattering_pdf * light_emission / light_pdf;
            }
        }
    }

    // Sample according to the material's brdf
    const auto scattered = Ray(context.hit_record.p, context.scatter_record.pdf_ptr->generate());
    const auto brdf_pdf = context.scatter_record.pdf_ptr->value(scattered.direction());

    if (brdf_pdf > 0) {
        const double scatteringPdf = context.hit_record.mat->scattering_pdf(
            context.incoming_ray, context.hit_record, scattered);
        const Color sampleColor = ray_color_function(scattered, context.remaining_depth);

        auto light_ptr_for_weight = make_shared<HittablePDF>(context.lights, context.hit_record.p);
        double light_pdf_for_this_direction = light_ptr_for_weight->value(scattered.direction());

        // POWER HEURISTIC (beta = 2)
        double light_pdf_2 = light_pdf_for_this_direction * light_pdf_for_this_direction;
        double brdf_pdf_2 = brdf_pdf * brdf_pdf;
        double weight_pt = brdf_pdf_2 / (light_pdf_2 + brdf_pdf_2);

        colorFromScatter += weight_pt * (context.scatter_record.attenuation *
                                        scatteringPdf * sampleColor) / brdf_pdf;
    }

    return colorFromScatter;
}

Color MixtureSamplingStrategy::compute_scattered_color(
    const SamplingContext& context,
    const std::function<Color(const Ray&, int)>& rayColorFunc
) const {
    // Standard path tracing using a mixture of light and BRDF sampling
    const auto light_ptr = make_shared<HittablePDF>(context.lights, context.hit_record.p);
    const MixturePDF p(light_ptr, context.scatter_record.pdf_ptr);

    const auto scattered = Ray(context.hit_record.p, p.generate());
    const auto pdfValue = p.value(scattered.direction());

    const double scatteringPdf = context.hit_record.mat->scattering_pdf(
        context.incoming_ray, context.hit_record, scattered);

    const Color sampleColor = rayColorFunc(scattered, context.remaining_depth);

    return (context.scatter_record.attenuation * scatteringPdf * sampleColor) / pdfValue;
}
