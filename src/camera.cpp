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


void ForwardCamera::render_line(const Hittable &world, const Hittable &lights, size_t j) {
    for (size_t column = 0; column < imageWidth; ++column) {
        render_pixel(world, lights, j, column);
    }
}

inline uint64_t combine(const uint32_t seed, const uint32_t x, const uint32_t y) {
    auto combined = static_cast<uint64_t>(seed);
    combined = (combined << 32) | ((static_cast<uint64_t>(x & 0xFFFF) << 16) | (y & 0xFFFF));
    return combined;
}

void ForwardCamera::persist_color_to_data(const size_t row, const size_t column, const Color pixel_color) {
    const size_t idx = 3 * (column + row * imageWidth);

    imageData.data[idx]     = pixel_color.x();  // R
    imageData.data[idx + 1] = pixel_color.y();  // G
    imageData.data[idx + 2] = pixel_color.z();  // B
}

std::shared_ptr<SampleAggregator> ForwardCamera::render_pixel(const Hittable &world, const Hittable &lights,
                                                             const size_t row, const size_t column) {
    random_seed(combine(seed, row, column));

    const auto aggregator = samplerAggregator->create();
    aggregator->sample_from(pixelSamplerFactory, static_cast<double>(column), static_cast<double>(row));

    for (const Sample& sample : *aggregator) {
        Ray r = get_ray(sample.x, sample.y);

        const Color color = rayColor(r, static_cast<int>(maxDepth), world, lights);
        aggregator->insert_contribution(color);
    }

    const Color pixel_color = aggregator->aggregate();

    persist_color_to_data(row, column, pixel_color);

    return aggregator;
}


void ForwardCamera::render(const Hittable& world, const Hittable& lights) {
    initialize();

    for (int j = 0; j < imageHeight; j++) {
        std::clog << "\rScanlines remaining: " << (imageHeight - j) << ' ' << std::flush;
        render_line(world, lights, j);
    }
}

Color ForwardCamera::rayColor(const Ray& r, const int depth, const Hittable& world, const Hittable& lights) const {
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
        return scatterRecord.attenuation * rayColor(scatterRecord.skip_pdf_ray, depth - 1, world, lights);
    }

    // Delegate to the sampling strategy
    SamplingStrategy::SamplingContext ctx{r, rec, scatterRecord, world, lights, depth - 1};

    auto ray_color_function = [this, &world, &lights](const Ray& ray, int d) {
        return this->rayColor(ray, d, world, lights);
    };

    const ScatteredContribution contribution = samplingStrategy->compute_scattered_color(ctx, ray_color_function);
    Color colorFromScatter = contribution.color;

    return color_from_emission + colorFromScatter;
}

void ForwardParallelCamera::render(const Hittable &world, const Hittable &lights) {
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
                render_line(world, lights, j);
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

void CartographyCamera::render(const Hittable &world, const Hittable &lights) {
    initialize();
    render_pixel(world, lights, pixel_y, pixel_x);
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
std::shared_ptr<SampleAggregator> CartographyCamera::render_pixel(const Hittable &world, const Hittable &lights,
                                                                 const size_t row, const size_t column) {
    std::clog << "Rendering pixel @ " << column << ", " << row << std::endl;
    for (size_t y = 0 ; y < imageHeight ; ++y) {
        const double dy = static_cast<double>(y) / static_cast<double>(imageHeight) - .5;
        for (size_t x = 0 ; x < imageWidth ; ++x) {
            const double dx = static_cast<double>(x) / static_cast<double>(imageWidth) - .5;
            Ray r = get_ray(dx + static_cast<double>(column), dy + static_cast<double>(row));

            Color pixel_color = rayColor(r, static_cast<int>(maxDepth), world, lights);

            persist_color_to_data(row, column, pixel_color);
        }
    }

    return nullptr;
}

std::shared_ptr<SampleAggregator> BiasedForwardParallelCamera::render_pixel(
    const Hittable &world, const Hittable &lights, size_t row, size_t column) {
    const auto aggregator = samplerAggregator->create();
    aggregator->sample_from(pixelSamplerFactory, static_cast<double>(column), static_cast<double>(row));

    for (const Sample& sample : *aggregator) {
        Ray r = get_ray(sample.x, sample.y);

        size_t retries = 0;
        Color color;

        do {
            color = rayColor(r, static_cast<int>(maxDepth), world, lights);
        } while (color.near_zero() && ++retries < 20);
        aggregator->insert_contribution(color);
    }

    // while (aggregator-> has_next()) {
    //     Sample sample = aggregator->next();
    //
    //     Ray r = get_ray(sample.x, sample.y);
    //
    //     size_t retries = 0;
    //     Color color;
    //
    //     do {
    //         color = rayColor(r, static_cast<int>(maxDepth), world, lights);
    //     } while (color.near_zero() && ++retries < 20);
    //     aggregator->insert_contribution(color);
    // }

    const Color pixel_color = aggregator->aggregate();

    persist_color_to_data(row, column, pixel_color);

    return aggregator;
}

#ifdef FUNCTION_PARSING
FunctionCamera::FunctionCamera(shared_ptr<Function> function): ForwardParallelCamera(), function(function) {}


std::shared_ptr<SampleAggregator> FunctionCamera::render_pixel(const Hittable &world, const Hittable &lights, size_t row, size_t column) {
    random_seed(combine(seed, row, column));

    const auto aggregator = samplerAggregator->create();
    aggregator->sample_from(pixelSamplerFactory, static_cast<double>(column), static_cast<double>(row));
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

Color NormalCamera::rayColor(const Ray &r, int depth, const Hittable &world, const Hittable &lights) const {
    if (depth <= 0)
        return {0, 0, 0};

    HitRecord rec;
    // If the ray hits nothing, return the background color.
    if (!world.hit(r, Interval(0.001, infinity), rec))
        return background;

    const auto n = unit_vector(rec.normal);

    double red = (n.x() + 1.) / 2.;
    double green = (n.y() + 1.) / 2.;
    double blue = (n.z() + 1.) / 2.;

    return {red, green, blue};
}

void SinglePixelCamera::render(const Hittable &world, const Hittable &lights) {
    initialize();

    render_pixel(world, lights, pixel_y, pixel_x);

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

Color TestCamera::rayColor(const Ray &r, int depth, const Hittable &world, const Hittable &lights) const {
    if (-r.origin().x() + r.origin().y() > 0) {
        return {0, 0, 0};
    } else return {1, 1, 1};
}

/*Color FBVCamera::rayColor(const Ray &r, int depth, const Hittable &world, const Hittable &lights) const {
    HitRecord rec;
    // If the ray hits nothing, return the background color.
    if (!world.hit(r, Interval(0.001, infinity), rec))
        return background;

    ScatterRecord scatterRecord;
    const Color color_from_emission = rec.mat->emitted(r, rec, rec.u, rec.v, rec.p);

    if (!rec.mat->scatter(r, rec, scatterRecord))
        return color_from_emission;

    if (scatterRecord.skip_pdf) {
        return scatterRecord.attenuation * ForwardParallelCamera::rayColor(scatterRecord.skip_pdf_ray, depth - 1, world, lights);
    }

    // scattering

    // Delegate to the sampling strategy
    SamplingStrategy::SamplingContext ctx{r, rec, scatterRecord, world, lights, depth - 1};

    auto ray_color_function = [this, &world, &lights](const Ray& ray, int d) {
        return ForwardParallelCamera::rayColor(ray, d, world, lights);
    };

    // instancier un aggrégateur voronoi
    // et pour chacun des samples de cet aggrégateur
    // calculer l'échantillon
    // Color colorFromScatter = samplingStrategy->compute_scattered_color(ctx, ray_color_function);

    // Color contribution = color_from_emission + colorFromScatter;
    // et agréger les contributions





    Color colorFromScatter = samplingStrategy->compute_scattered_color(ctx, ray_color_function).color;

    return color_from_emission + colorFromScatter;
}*/

Color FBVCamera::rayColor(const Ray &r, int depth, const Hittable &world, const Hittable &lights) const {
    HitRecord rec;
    // If the ray hits nothing, return the background color.
    if (!world.hit(r, Interval(0.001, infinity), rec))
        return background;

    ScatterRecord scatterRecord;
    const Color color_from_emission = rec.mat->emitted(r, rec, rec.u, rec.v, rec.p);

    if (!rec.mat->scatter(r, rec, scatterRecord))
        return color_from_emission;

    if (scatterRecord.skip_pdf) {
        return scatterRecord.attenuation * far_ray_color(scatterRecord.skip_pdf_ray, depth - 1, world, lights);
    }

    // scattering

    // Delegate to the sampling strategy
    SamplingStrategy::SamplingContext ctx{r, rec, scatterRecord, world, lights, depth - 1};

    auto ray_color_function = [this, &world, &lights](const Ray& ray, int d) {
        return far_ray_color(ray, d, world, lights);
    };

    // instancier un aggrégateur voronoi
    // et pour chacun des samples de cet aggrégateur
    // calculer l'échantillon
    // Color colorFromScatter = samplingStrategy->compute_scattered_color(ctx, ray_color_function);

    // Color contribution = color_from_emission + colorFromScatter;
    // et agréger les contributions

    FirstBounceVoronoi ag;
    Traits traits(Point_3(0, 0, 0), 1.0); // Unit sphere
    auto dt = SDT(traits);

    std::vector<Vec3> directions;
    std::vector<Color> contributions;
    std::vector<double> weights;
    double total_area = 0.;

    size_t N_SAMPLES = 250;

    for (int i = 0 ; i < N_SAMPLES ; ++i) {
        const ScatteredContribution contribution = samplingStrategy->compute_scattered_color(ctx, ray_color_function);
        const Color colorFromScatter = contribution.color;
        auto direction = contribution.outgoing.direction();
        direction /= direction.length();
        directions.push_back(direction);
        contributions.push_back(colorFromScatter);
        dt.insert(Point_3(direction.x(), direction.y(), direction.z()));
        // std::cout << direction << " -- " << direction.length2() << std::endl;
    }

    for (auto &direction: directions) {
        const Vec3 ref = reflect(direction, rec.normal);
        dt.insert(Point_3(ref.x(), ref.y(), ref.z()));
    }

    for (auto v = dt.finite_vertices_begin(); v != dt.finite_vertices_end(); ++v) {
        Point_3 site = v->point();
        if (site.z() < 0) continue; // only useful contributions

        double cell_solid_angle = 0.0;
        SDT::Face_circulator fc = dt.incident_faces(v), done(fc);

        std::vector<Point_3> voronoi_vertices;
        if (fc != nullptr) {
            do {
                // if samples are drawn from a hemisphere, dt.is_infinite() may return true
                if (!dt.is_infinite(fc)) {
                    Point_3 p = ag.get_spherical_dual(fc);
                    voronoi_vertices.push_back(p);
                }
            } while (++fc != done);
        }
        if (!voronoi_vertices.empty()) {
            for (std::size_t i = 0; i < voronoi_vertices.size(); ++i) {
                const Point_3& v1 = voronoi_vertices[i];
                const Point_3& v2 = voronoi_vertices[(i + 1) % voronoi_vertices.size()];
                cell_solid_angle += ag.solid_angle(site, v1, v2);
                weights.push_back(cell_solid_angle);
            }
        }
        total_area += cell_solid_angle;
    }

    // std::cout << "total area  " << total_area << std::endl;
    // std::exit(0);

    Color colorFromScatter(0,0,0);

    for (int i = 0 ; i < N_SAMPLES ; ++i) {
        colorFromScatter += weights[i] * contributions[i];
    }

    colorFromScatter /= total_area;


    return color_from_emission + colorFromScatter;
}

Color FBVCamera::far_ray_color(const Ray& r, const int depth, const Hittable& world, const Hittable& lights) const {
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
        return scatterRecord.attenuation * far_ray_color(scatterRecord.skip_pdf_ray, depth - 1, world, lights);
    }

    // Delegate to the sampling strategy
    SamplingStrategy::SamplingContext ctx{r, rec, scatterRecord, world, lights, depth - 1};

    auto ray_color_function = [this, &world, &lights](const Ray& ray, int d) {
        return this->far_ray_color(ray, d, world, lights);
    };

    const ScatteredContribution contribution = samplingStrategy->compute_scattered_color(ctx, ray_color_function);
    Color colorFromScatter = contribution.color;

    return color_from_emission + colorFromScatter;
}
