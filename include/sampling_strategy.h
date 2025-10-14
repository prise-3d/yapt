//
// Created by franck on 13/10/25.
//

#ifndef YAPT_SAMPLING_STRATEGY_H
#define YAPT_SAMPLING_STRATEGY_H

#include "yapt.h"
#include "hittable.h"
#include "material.h"
#include <functional>

class SamplingStrategy {
public:
    virtual ~SamplingStrategy() = default;

    /**
     * Context structure, used for computing scattered light
     */
    struct SamplingContext {
        const Ray& incoming_ray;           // The ray that hit the surface
        const HitRecord& hit_record;       // Information about the hit point
        const ScatterRecord& scatter_record; // BRDF information from the material
        const Hittable& world;             // The scene geometry
        const Hittable& lights;            // Light sources for sampling
        int remaining_depth;               // Remaining ray bounces
    };

    virtual Color compute_scattered_color(
        const SamplingContext& context,
        const std::function<Color(const Ray&, int)>& ray_color_function
    ) const = 0;
};

class NEESamplingStrategy : public SamplingStrategy {
public:
    ~NEESamplingStrategy() override = default;

    Color compute_scattered_color(
        const SamplingContext& context,
        const std::function<Color(const Ray&, int)>& ray_color_function
    ) const override;
};

class MixtureSamplingStrategy : public SamplingStrategy {
public:
    ~MixtureSamplingStrategy() override = default;

    Color compute_scattered_color(
        const SamplingContext& context,
        const std::function<Color(const Ray&, int)>& rayColorFunc
    ) const override;
};

#endif //YAPT_SAMPLING_STRATEGY_H
