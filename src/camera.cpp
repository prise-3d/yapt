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
#include "camera.h"
#include "material.h"
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <condition_variable>
#include <memory>
#include <scene.h>
#include <functional>

void Camera::initialize() {
    imageHeight = static_cast<size_t>(static_cast<double>(imageWidth) / aspect_ratio);
    imageHeight = (imageHeight < 1) ? 1 : imageHeight;
    center = lookFrom;

    // Determine viewport dimensions.
    const auto theta = degrees_to_radians(vfov);
    const auto h = tan(theta/2);
    const auto viewportHeight = 2 * h * focusDist;
    const auto viewportWidth = viewportHeight * (static_cast<double>(imageWidth) / static_cast<double>(imageHeight));

    // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
    w = unit_vector(lookFrom - lookAt);
    u = unit_vector(cross(vup, w));
    v = cross(w, u);

    // Calculate the vectors across the horizontal and down the vertical viewport edges.
    Vec3 viewportU = viewportWidth * u;    // Vector across viewport horizontal edge
    Vec3 viewportV = viewportHeight * -v;  // Vector down viewport vertical edge

    // Calculate the horizontal and vertical delta vectors from pixel to pixel.
    pixel_delta_u = viewportU / static_cast<double>(imageWidth);
    pixel_delta_v = viewportV / static_cast<double>(imageHeight);

    // Calculate the location of the upper left pixel.
    const auto viewportUpperLeft = center - (focusDist * w) - viewportU / 2 - viewportV / 2;
    pixel00_loc = viewportUpperLeft + 0.5 * (pixel_delta_u + pixel_delta_v);

    // Calculate the camera defocus disk basis vectors.
    const auto defocusRadius = focusDist * tan(degrees_to_radians(defocusAngle / 2));
    defocusDiskU = u * defocusRadius;
    defocusDiskV = v * defocusRadius;

    imageData.data = std::vector<double>(imageWidth * imageHeight * 3);
    imageData.width = imageWidth;
    imageData.height = imageHeight;
}

/**
 * Gets a ray from a fractional pixel position in the pixel grid
 * @param x x (fractional) coordinate of a pixel. 0 <= x < imageWidth
 * @param y y (fractional) coordinate of a pixel. 0 <= y < imageHeight
 * @return a ray traversing this fractional pixel.
 */
Ray Camera::get_ray(double x, double y) const {
    auto pixelSample = pixel00_loc +
                       x * pixel_delta_u +
                       y * pixel_delta_v;

    auto rayOrigin = (defocusAngle <= 0) ? center : defocusDiskSample();
    auto rayDirection = pixelSample - rayOrigin;

    return {rayOrigin, rayDirection};
}

/**
 * Samples a point from a defocus disk around the camera center
 * @return a random point of the defocus disk
 */
Point3 Camera::defocusDiskSample() const {
    // Returns a random point in the camera defocus disk.
    auto p = random_in_unit_disk();
    return center + (p[0] * defocusDiskU) + (p[1] * defocusDiskV);
}


void ForwardCamera::render_line(const Scene &scene, size_t j) {
    for (size_t column = 0; column < imageWidth; ++column) {
        render_pixel(scene, j, column);
    }
}

inline uint64_t combine(const uint32_t seed, const uint32_t x, const uint32_t y) {
    auto combined = static_cast<uint64_t>(seed);
    combined = (combined << 32) | ((static_cast<uint64_t>(x & 0xFFFF) << 16) | (y & 0xFFFF));
    std::cout << combined << std::endl;
    return combined;
}

void ForwardCamera::persist_color_to_data(const size_t row, const size_t column, const Color pixel_color) {
    const size_t idx = 3 * (column + row * imageWidth);

    imageData.data[idx]     = pixel_color.x();  // R
    imageData.data[idx + 1] = pixel_color.y();  // G
    imageData.data[idx + 2] = pixel_color.z();  // B
}

std::shared_ptr<SampleAggregator> ForwardCamera::render_pixel(const Scene &scene,
                                                             const size_t row, const size_t column) {
    random_seed(combine(seed, row, column));

    const auto aggregator = aggregator_factory->create();
    aggregator->sample_from(sampler_factory, static_cast<double>(column), static_cast<double>(row));

    for (const Sample& sample : *aggregator) {
        Ray r = get_ray(sample.x, sample.y);

        const Color color = ray_evaluator->evaluate(r, static_cast<int>(maxDepth), scene, background);
        aggregator->insert_contribution(color);
    }

    const Color pixel_color = aggregator->aggregate();

    persist_color_to_data(row, column, pixel_color);

    return aggregator;
}


void ForwardCamera::render(const Scene &scene) {
    initialize();

    for (int j = 0; j < imageHeight; j++) {
        std::clog << "\rScanlines remaining: " << (imageHeight - j) << ' ' << std::flush;
        render_line(scene, j);
    }
}

void ForwardParallelCamera::render(const Scene &scene) {
    initialize();

    std::mutex queue_mutex;

    // Available threads
    if (numThreads <= 0) numThreads = std::thread::hardware_concurrency();

    std::cout << "rendering using " << numThreads << " threads" << std::endl;
    std::vector<std::thread> threads(numThreads);

    std::queue<std::pair<int, int>> taskQueue;

    for (size_t start_j = 0 ; start_j < imageHeight ; start_j += linesPerBatch) {
        taskQueue.emplace(start_j, std::min(start_j + linesPerBatch - 1, imageHeight-1));
    }
    std::clog << std::endl;

    // Here we declare a function to process a task (render a few lines of the image)
    auto processSegment = [&]() {

        while (true) {
            std::pair<int, int> task;
            size_t remainingTasks;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                if (taskQueue.empty()) {
                    break;  // All the tasks are done!
                }
                task = taskQueue.front();
                taskQueue.pop();
                remainingTasks = taskQueue.size();
            }

            std::cout << "\rTasks remaining: " << remainingTasks << "   " << std::flush;

            const int start_j = task.first;
            const int end_j = task.second;

            for (int j = start_j; j <= end_j; ++j) {
                render_line(scene, j);
            }
        }
    };

    // start the threads
    for (int t = 0; t < numThreads; ++t) {
        threads[t] = std::thread(processSegment);
    }

    // Waiting for the threads to finish their tasks
    for (auto& t : threads) {
        t.join();
    }
    std::clog << std::endl;
}

CartographyCamera::CartographyCamera(const size_t pixel_x, const size_t pixel_y): pixel_x(pixel_x), pixel_y(pixel_y) {}

void CartographyCamera::render(const Scene &scene) {
    initialize();
    render_pixel(scene, pixel_y, pixel_x);
}

void CartographyCamera::initialize() {
    ForwardCamera::initialize();
}

/**
 * Render a pixel cartography. We assume for now that the pixel is uniformly sampled
 * @param world
 * @param lights
 * @param row
 * @param column
 */
std::shared_ptr<SampleAggregator> CartographyCamera::render_pixel(const Scene& scene,
                                                                 const size_t row, const size_t column) {
    std::clog << "Rendering pixel @ " << column << ", " << row << std::endl;
    for (size_t y = 0 ; y < imageHeight ; ++y) {
        const double dy = static_cast<double>(y) / static_cast<double>(imageHeight) - .5;
        for (size_t x = 0 ; x < imageWidth ; ++x) {
            const double dx = static_cast<double>(x) / static_cast<double>(imageWidth) - .5;
            Ray r = get_ray(dx + static_cast<double>(column), dy + static_cast<double>(row));

            Color pixel_color = ray_evaluator->evaluate(r, static_cast<int>(maxDepth), scene, background);

            persist_color_to_data(row, column, pixel_color);
        }
    }

    return nullptr;
}

std::shared_ptr<SampleAggregator> BiasedForwardParallelCamera::render_pixel(
    const Scene &scene, size_t row, size_t column) {
    const auto aggregator = aggregator_factory->create();
    aggregator->sample_from(sampler_factory, static_cast<double>(column), static_cast<double>(row));

    for (const Sample& sample : *aggregator) {
        Ray r = get_ray(sample.x, sample.y);

        size_t retries = 0;
        Color color;

        do {
            color = ray_evaluator->evaluate(r, static_cast<int>(maxDepth), scene, background);
        } while (color.near_zero() && ++retries < 20);
        aggregator->insert_contribution(color);
    }

    const Color pixel_color = aggregator->aggregate();

    persist_color_to_data(row, column, pixel_color);

    return aggregator;
}

#ifdef FUNCTION_PARSING
FunctionCamera::FunctionCamera(shared_ptr<Function> function): ForwardParallelCamera(), function(function) {}


std::shared_ptr<SampleAggregator> FunctionCamera::render_pixel(const Hittable &world, const Hittable &lights, size_t row, size_t column) {
    random_seed(combine(seed, row, column));

    const auto aggregator = aggregator_factory->create();
    aggregator->sample_from(sampler_factory, static_cast<double>(column), static_cast<double>(row));
    for (const Sample& sample : *aggregator) {
        const double value = function->compute(sample.dx, sample.dy);
        const Color color(value, value, value);
        aggregator->insert_contribution(color);
    }

    const Color pixel_color = aggregator->aggregate();

    persist_color_to_data(row, column, pixel_color);

    return aggregator;
}
#endif

void SinglePixelCamera::render(const Scene &scene) {
    initialize();

    render_pixel(scene, pixel_y, pixel_x);

    const size_t idx = 3 * (pixel_x + pixel_y * imageWidth);

    imageData.data[0] = imageData.data[idx];
    imageData.data[1] = imageData.data[idx + 1];
    imageData.data[2] = imageData.data[idx + 2];

    imageHeight = 1;
    imageWidth = 1;

    imageData.data.resize(3);
    imageData.width = 1;
    imageData.height = 1;
}

SinglePixelCamera::SinglePixelCamera(const size_t pixel_x, const size_t pixel_y): pixel_x(pixel_x), pixel_y(pixel_y) {}

Ray TestCamera::get_ray(const double x, const double y) const {
    double ex, ey;
    const double dx = modf(x, &ex);
    const double dy = modf(y, &ey);

    return {Point3(dx, dy, 0), Vec3(0, 0, 0)};
}

RLCamera::RLCamera(
    const std::size_t warmup_phases,
    const std::size_t exploitation_phases,
    const std::size_t voxel_grid_resolution) :
        warmup_phases(warmup_phases),
        exploitation_phases(exploitation_phases),
        voxelGrid(Vec3(-.01,-.01,-.01),Vec3(555.01,555.01,555.01), voxel_grid_resolution) {}

void RLCamera::render(const Scene& scene) {
    initialize();
    maxDepth = 10;

    // warmup
    std::cout << "WARMUP (x" << warmup_phases << ")" << std::endl;
    if (!record_warmup) {
        std::cout << "(warmup contributions NOT recorded)" << std::endl;
    }
    for (std::size_t i = 0 ; i < warmup_phases ; ++i) {
        std::cout << "  PASS " << i << "          " << std::endl;
        for (std::size_t row = 0 ; row < imageHeight ; ++row) {
            for (std::size_t column = 0 ; column < imageWidth ; ++column) {
                random_seed(combine(seed, row + i * imageHeight, column ));
                const auto aggregator = aggregator_factory->create();
                aggregator->sample_from(sampler_factory, static_cast<double>(column), static_cast<double>(row));
                for (const Sample& sample : *aggregator) {
                    Ray r = get_ray(sample.x, sample.y);
                    std::vector<Point3> path_positions;
                    const Color contribution = evaluate(r, static_cast<int>(maxDepth), scene, path_positions);
                    aggregator->insert_contribution(contribution);
                    for (const auto& position : path_positions) {
                        double radiance_value = std::max(contribution.x(), std::max(contribution.y(), contribution.z()));
                        voxelGrid.record(position, radiance_value);
                    }
                }
                if (record_warmup) {
                    const Color pixel_color = aggregator->aggregate();
                    persist_color_to_data(row, column, pixel_color);
                }
            }
        }
    }

    double best = -1;
    int best_index = -1;
    int count = 0;
    const std::size_t voxel_count = voxelGrid.voxel_count();

    for (int i = 0 ; i  < voxel_count ; ++i) {
        const auto contrib = voxelGrid.radiance[i];
        if (contrib > best) {
            best = contrib;
            best_index = i;
        }
        if (contrib != 0) {
            count++;
        }
    }
    std::cout << "Best: " << best << std::endl;
    std::cout << "Best Index: " << best_index << std::endl;
    std::cout << "Non zero voxels: " << count << std::endl;

    // exploit
    std::cout << std::endl << std::endl << "EXPLOITATION (x" << exploitation_phases << ")          " << std::endl;
    for (std::size_t i = 0 ; i < exploitation_phases ; ++i) {
        std::cout << "  PASS " << i << "          " << std::endl;
        for (std::size_t row = 0 ; row < imageHeight ; ++row) {
            for (std::size_t column = 0 ; column < imageWidth ; ++column) {
                random_seed(combine(seed, row + (i + warmup_phases) * imageHeight, column));
                const auto aggregator = aggregator_factory->create();
                aggregator->sample_from(sampler_factory, static_cast<double>(column), static_cast<double>(row));
                for (const Sample& sample : *aggregator) {
                    Ray r = get_ray(sample.x, sample.y);
                    std::vector<Point3> path_positions;
                    const Color contribution = guide_and_evaluate(r, static_cast<int>(maxDepth), scene, path_positions);
                    aggregator->insert_contribution(contribution);
                    for (const auto& position : path_positions) {
                        double radiance_value = std::max(contribution.x(), std::max(contribution.y(), contribution.z()));
                        voxelGrid.record(position, radiance_value);
                    }
                }
                const Color pixel_color = aggregator->aggregate();
                persist_color_to_data(row, column, pixel_color);
            }
        }
    }

    std::size_t k = 0;

    const auto passes_count = static_cast<double>(record_warmup ? warmup_phases + exploitation_phases : exploitation_phases);
    for (int row = 0 ; row < imageHeight ; ++row)
    {
        for (int column = 0 ; column < imageWidth ; ++column)
        {
            imageData.data[k++] /= passes_count;  // R
            imageData.data[k++] /= passes_count;  // G
            imageData.data[k++] /= passes_count;  // B
        }
    }

}

Color RLCamera::evaluate(const Ray& ray, const int depth, const Scene& scene, std::vector<Point3> &path_positions) {
    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0)
        return {0, 0, 0};

    HitRecord rec;
    // If the ray hits nothing, return the background color.
    if (!scene.geometry.hit(ray, Interval(0.001, infinity), rec))
        return {0, 0, 0}; // background

    path_positions.push_back(rec.p);

    ScatterRecord scatterRecord;
    const Color color_from_emission = rec.mat->emitted(ray, rec, rec.u, rec.v, rec.p);

    if (!rec.mat->scatter(ray, rec, scatterRecord))
        return color_from_emission;

    if (scatterRecord.skip_pdf) {
        return scatterRecord.attenuation * evaluate(scatterRecord.skip_pdf_ray, depth - 1, scene, path_positions);
    }

    // const ScatteringStrategy::ScatteringContext context{ray, rec, scatterRecord, scene, depth - 1};

    const auto light_ptr = make_shared<HittablePDF>(scene.lights, rec.p);
    const MixturePDF p(light_ptr, scatterRecord.pdf_ptr);

    auto scattered = Ray(rec.p, p.generate());
    const auto pdfValue = p.value(scattered.direction());

    const double scatteringPdf = rec.mat->scattering_pdf(
        ray, rec, scattered);

    const Color sampleColor = evaluate(scattered, depth - 1, scene, path_positions);

    const auto colorFromScatter=
        scatterRecord.attenuation * scatteringPdf * sampleColor / pdfValue;

    return color_from_emission + colorFromScatter;
}

Color RLCamera::guide_and_evaluate(const Ray& ray, const int depth, const Scene& scene, std::vector<Point3> &path_positions) {
    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0)
        return {0, 0, 0};

    HitRecord rec;
    // If the ray hits nothing, return the background color.
    if (!scene.geometry.hit(ray, Interval(0.001, infinity), rec))
        return {0, 0, 0}; // background

    path_positions.push_back(rec.p);

    ScatterRecord scatterRecord;
    const Color color_from_emission = rec.mat->emitted(ray, rec, rec.u, rec.v, rec.p);

    if (!rec.mat->scatter(ray, rec, scatterRecord))
        return color_from_emission;

    if (scatterRecord.skip_pdf) {
        return scatterRecord.attenuation * guide_and_evaluate(scatterRecord.skip_pdf_ray, depth - 1, scene, path_positions);
    }

    const auto light_ptr = make_shared<HittablePDF>(scene.lights, rec.p);
    const MixturePDF p(light_ptr, scatterRecord.pdf_ptr);

    Ray scattered;
    double pdfValue = 1;
    double best_contribution = -1;

    for (std::size_t x = 0 ; x < 4 ; ++x) {
        auto temp_ray = Ray(rec.p, p.generate());
        const auto temp_value = p.value(temp_ray.direction());
        HitRecord temp_record;

        if (scene.geometry.hit(ray, Interval(0.001, infinity), temp_record)) {
            const auto temp_point = temp_record.p;
            const auto contribution = voxelGrid.lookup(temp_point);
            if (contribution > best_contribution) {
                best_contribution = contribution;
                scattered = temp_ray;
                pdfValue = temp_value;
            }
        }
    }

    const double scatteringPdf = rec.mat->scattering_pdf(
        ray, rec, scattered);

    const Color sampleColor = evaluate(scattered, depth - 1, scene, path_positions);

    const auto colorFromScatter=
        scatterRecord.attenuation * scatteringPdf * sampleColor / pdfValue;

    return color_from_emission + colorFromScatter;
}

void RLCamera::persist_color_to_data(const std::size_t row, const std::size_t column, const Color pixel_color) {
    const size_t idx = 3 * (column + row * imageWidth);

    imageData.data[idx]     += pixel_color.x();  // R
    imageData.data[idx + 1] += pixel_color.y();  // G
    imageData.data[idx + 2] += pixel_color.z();  // B
}


