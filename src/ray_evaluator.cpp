//
// Created by franck on 30/01/2026.
//

#include "ray_evaluator.h"

Color SimpleRayEvaluator::evaluate(const Ray &r, const int depth, const Scene &scene, const Color &background) {

    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0)
        return {0, 0, 0};

    HitRecord rec;
    // If the ray hits nothing, return the background color.
    if (!scene.geometry.hit(r, Interval(0.001, infinity), rec))
        return background;

    ScatterRecord scatterRecord;
    const Color color_from_emission = rec.mat->emitted(r, rec, rec.u, rec.v, rec.p);

    if (!rec.mat->scatter(r, rec, scatterRecord))
        return color_from_emission;

    if (scatterRecord.skip_pdf) {
        return scatterRecord.attenuation * evaluate(scatterRecord.skip_pdf_ray, depth - 1, scene, background);
    }

    // Delegate to the sampling strategy
    SamplingStrategy::SamplingContext ctx{r, rec, scatterRecord, scene, depth - 1};

    auto ray_color_function = [this, &scene, background](const Ray& ray, const int d) {
        return this->evaluate(ray, d, scene, background);
    };

    const ScatteredContribution contribution = sampling_strategy->compute_scattered_color(ctx, ray_color_function);
    const Color colorFromScatter = contribution.color;

    return color_from_emission + colorFromScatter;

}
