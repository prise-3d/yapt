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
