//
// Created by franck on 29/01/2026.
//

#ifndef YAPT_SCATTERING_STRATEGY_H
#define YAPT_SCATTERING_STRATEGY_H


#include "yapt.h"
#include "sampling_strategy.h"

class RayEvaluator {
public:
    explicit RayEvaluator(const shared_ptr<SamplingStrategy> &sampling_strategy) : sampling_strategy(sampling_strategy) {}

    virtual ~RayEvaluator() = default;
    virtual Color evaluate(
        const Ray&,
        const int depth,
        const Hittable& world,
        const Hittable& lights,
        const Color background
    ) = 0;
    shared_ptr<SamplingStrategy> sampling_strategy;
};

class SimpleRayEvaluator : public RayEvaluator {
public:
    explicit SimpleRayEvaluator(const shared_ptr<SamplingStrategy> &sampling_strategy) : RayEvaluator(sampling_strategy) {}

    ~SimpleRayEvaluator() override = default;

    Color evaluate(
            const Ray &r,
            const int depth,
            const Hittable &world,
            const Hittable &lights,
            const Color background
            )
    override {
        // If we've exceeded the ray bounce limit, no more light is gathered.
        if (depth <= 0)
                return {0, 0, 0};

        HitRecord rec;
        // If the ray hits nothing, return the background color.
        if (!world.hit(r, Interval(0.001, infinity), rec))
                return background;

        ScatterRecord scatterRecord;
        const Color color_from_emission = rec.mat->emitted(r, rec, rec.u, rec.v, rec.p);

        if (!rec.mat->scatter(r, rec, scatterRecord))
                return color_from_emission;

        if (scatterRecord.skip_pdf) {
                return scatterRecord.attenuation * evaluate(scatterRecord.skip_pdf_ray, depth - 1, world, lights, background);
        }

        // Delegate to the sampling strategy
        SamplingStrategy::SamplingContext ctx{r, rec, scatterRecord, world, lights, depth - 1};

        auto ray_color_function = [this, &world, &lights, background](const Ray& ray, const int d) {
                return this->evaluate(ray, d, world, lights, background);
        };

        const ScatteredContribution contribution = sampling_strategy->compute_scattered_color(ctx, ray_color_function);
        const Color colorFromScatter = contribution.color;

        return color_from_emission + colorFromScatter;
    }
};

#endif //YAPT_SCATTERING_STRATEGY_H