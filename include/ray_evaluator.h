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

class SimpleRayEvaluator final : public SamplingRayEvaluator {
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

class StepRayEvaluator : public SamplingRayEvaluator {
public:
    explicit StepRayEvaluator(const shared_ptr<SamplingStrategy> &sampling_strategy) : SamplingRayEvaluator(sampling_strategy) {}
    StepRayEvaluator(const shared_ptr<SamplingStrategy> &sampling_strategy, const shared_ptr<RayEvaluator> &next_step) :
        SamplingRayEvaluator(sampling_strategy), next_step(next_step) {}

    shared_ptr<RayEvaluator> next_step;
};

class NestedRayEvaluator final : public StepRayEvaluator {
public:
    NestedRayEvaluator(const shared_ptr<SamplingStrategy> &sampling_strategy, size_t sample_size) : StepRayEvaluator(sampling_strategy), sample_size(sample_size) {}
    NestedRayEvaluator(const shared_ptr<SamplingStrategy> &sampling_strategy, const shared_ptr<RayEvaluator> &next_step, size_t sample_size) : StepRayEvaluator(sampling_strategy, next_step), sample_size(sample_size) {}

    Color evaluate(const Ray &, const int depth, const Scene &scene, const Color &background) override;

    size_t sample_size;
};

class CVorNestedRayEvaluator final : public StepRayEvaluator {
public:
    CVorNestedRayEvaluator(const shared_ptr<SamplingStrategy> &sampling_strategy, size_t sample_size) : StepRayEvaluator(sampling_strategy), sample_size(sample_size) {}
    CVorNestedRayEvaluator(const shared_ptr<SamplingStrategy> &sampling_strategy, const shared_ptr<RayEvaluator> &next_step, size_t sample_size) : StepRayEvaluator(sampling_strategy, next_step), sample_size(sample_size) {}

    Color evaluate(const Ray &, const int depth, const Scene &scene, const Color &background) override;

    size_t sample_size;
};

#endif //YAPT_SCATTERING_STRATEGY_H