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

#ifndef YAPT_VOXEL_GRID_H
#define YAPT_VOXEL_GRID_H

#include <algorithm>
#include "Vec3.h"

class VoxelGrid
{
public:

    VoxelGrid(const Vec3 &bbox_min, const Vec3 &bbox_max, const std::size_t resolution) :
        bbox_min(bbox_min), bbox_max(bbox_max), extent(bbox_max - bbox_min), resolution(resolution), radiance(resolution * resolution * resolution), visits(resolution * resolution * resolution)
    {}

    [[nodiscard]] std::size_t position_to_index(const Point3 &position) const {
        auto n = (position - bbox_min);
        n.e[0] = std::clamp(n.x() / extent.x(), 0.0, 0.9999);
        n.e[1] = std::clamp(n.y() / extent.y(), 0.0, 0.9999);
        n.e[2] = std::clamp(n.z() / extent.z(), 0.0, 0.9999);

        const auto i = static_cast<std::size_t>(std::floor(resolution * n.x()));
        const auto j = static_cast<std::size_t>(std::floor(resolution * n.y()));
        const auto k = static_cast<std::size_t>(std::floor(resolution * n.z()));

        return i + resolution * (j + resolution * k);
    }

    void record(const Point3 &position, const double &radiance_value) {
        const auto index = position_to_index(position);
        radiance[index] = radiance_value;
        visits[index] += 1;
    }

    [[nodiscard]] double lookup(const Point3 &position) const {
        const auto index = position_to_index(position);
        const auto total_visits = visits[index];
        return total_visits > 0 ? radiance[index] / static_cast<double>(total_visits) : 0.0;
    }

    void merge(const VoxelGrid& other) {
        for (std::size_t i = 0; i < radiance.size(); ++i) {
            radiance[i] += other.radiance[i];
            visits[i] += other.visits[i];
        }
    }

    Vec3 bbox_min;
    Vec3 bbox_max;
    std::vector<double> radiance;
    std::vector<std::size_t> visits;
    Vec3 extent;
    size_t resolution;
};

#endif //YAPT_VOXEL_GRID_H