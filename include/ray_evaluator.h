//
// Created by franck on 29/01/2026.
//

#ifndef YAPT_SCATTERING_STRATEGY_H
#define YAPT_SCATTERING_STRATEGY_H


#include "yapt.h"
#include "sampling_strategy.h"
#include "scene.h"

class RayEvaluator {
public:
    RayEvaluator() = default;
    virtual ~RayEvaluator() = default;
    virtual Color evaluate(
        const Ray&,
        const int depth,
        const Scene& scene,
        const Color &background
    ) = 0;
};

class SamplingRayEvaluator : public RayEvaluator {
public:
    explicit SamplingRayEvaluator(const shared_ptr<SamplingStrategy> &sampling_strategy) : sampling_strategy(sampling_strategy) {}
protected:
    shared_ptr<SamplingStrategy> sampling_strategy;
};

class SimpleRayEvaluator : public SamplingRayEvaluator {
public:
    explicit SimpleRayEvaluator(const shared_ptr<SamplingStrategy> &sampling_strategy) : SamplingRayEvaluator(sampling_strategy) {}

    ~SimpleRayEvaluator() override = default;

    Color evaluate(
            const Ray &r,
            const int depth,
            const Scene &scene,
            const Color &background
            )
    override;
};

class NormalRayEvaluator final : public RayEvaluator {
public:
    NormalRayEvaluator() = default;
    Color evaluate(const Ray &, const int depth, const Scene &scene, const Color &background) override;
};

#endif //YAPT_SCATTERING_STRATEGY_H