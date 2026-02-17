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

#ifndef YAPT_SCATTERING_STRATEGY_H
#define YAPT_SCATTERING_STRATEGY_H


#include "yapt.h"
#include "scattering_strategy.h"
#include "scene.h"
#include "spherical_voronoi.h"

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
    explicit SamplingRayEvaluator(const shared_ptr<ScatteringStrategy> &sampling_strategy) : sampling_strategy(sampling_strategy) {}
protected:
    shared_ptr<ScatteringStrategy> sampling_strategy;
};

class SimpleRayEvaluator final : public SamplingRayEvaluator {
public:
    explicit SimpleRayEvaluator(const shared_ptr<ScatteringStrategy> &sampling_strategy) : SamplingRayEvaluator(sampling_strategy) {}

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
    explicit StepRayEvaluator(const shared_ptr<ScatteringStrategy> &sampling_strategy) : SamplingRayEvaluator(sampling_strategy) {}
    StepRayEvaluator(const shared_ptr<ScatteringStrategy> &sampling_strategy, const shared_ptr<RayEvaluator> &next_step) :
        SamplingRayEvaluator(sampling_strategy), next_step(next_step) {}

    shared_ptr<RayEvaluator> next_step;
};

class NestedRayEvaluator final : public StepRayEvaluator {
public:
    NestedRayEvaluator(const shared_ptr<ScatteringStrategy> &sampling_strategy, size_t sample_size) : StepRayEvaluator(sampling_strategy), sample_size(sample_size) {}
    NestedRayEvaluator(const shared_ptr<ScatteringStrategy> &sampling_strategy, const shared_ptr<RayEvaluator> &next_step, size_t sample_size) : StepRayEvaluator(sampling_strategy, next_step), sample_size(sample_size) {}

    Color evaluate(const Ray &, const int depth, const Scene &scene, const Color &background) override;

    size_t sample_size;
};

class CVorNestedRayEvaluator final : public StepRayEvaluator {
public:
    CVorNestedRayEvaluator(const shared_ptr<ScatteringStrategy> &sampling_strategy, size_t sample_size) : StepRayEvaluator(sampling_strategy), sample_size(sample_size) {}
    CVorNestedRayEvaluator(const shared_ptr<ScatteringStrategy> &sampling_strategy, const shared_ptr<RayEvaluator> &next_step, size_t sample_size) : StepRayEvaluator(sampling_strategy, next_step), sample_size(sample_size) {}

    Color evaluate(const Ray &, const int depth, const Scene &scene, const Color &background) override;

    size_t sample_size;
};

class ObservableCVorNestedRayEvaluator final : public StepRayEvaluator {
public:
    ObservableCVorNestedRayEvaluator(const shared_ptr<ScatteringStrategy> &sampling_strategy, size_t sample_size, const std::vector<shared_ptr<SphericalVoronoiIntegratorObserver>> &observers) : StepRayEvaluator(sampling_strategy), sample_size(sample_size), observers(observers) {}
    ObservableCVorNestedRayEvaluator(const shared_ptr<ScatteringStrategy> &sampling_strategy, const shared_ptr<RayEvaluator> &next_step, size_t sample_size, std::vector<shared_ptr<SphericalVoronoiIntegratorObserver>> &observers) : StepRayEvaluator(sampling_strategy, next_step), sample_size(sample_size), observers(observers) {}

    Color evaluate(const Ray &, const int depth, const Scene &scene, const Color &background) override;

    size_t sample_size;
    std::vector<shared_ptr<SphericalVoronoiIntegratorObserver>> observers;

    void clear_observers();
};

#endif //YAPT_SCATTERING_STRATEGY_H