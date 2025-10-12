//
// Created by franck on 09/06/24.
//

#include "yapt.h"
#include "camera.h"
#include "material.h"
#include "path.h"
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
    randomSeed(combine(seed, row, column));

    const auto aggregator = samplerAggregator->create();
    aggregator->sample_from(pixelSamplerFactory, static_cast<double>(column), static_cast<double>(row));
    aggregator->traverse();

    while (aggregator-> has_next()) {
        Sample sample = aggregator->next();

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

    Color colorFromScatter{0, 0, 0};

    // Next Event Estimation (NEE)
    if (this->use_nee) {

        // Sample a point on the light
        auto light_ptr = make_shared<HittablePDF>(lights, rec.p);
        Ray light_ray(rec.p, light_ptr->generate());
        double light_pdf = light_ptr->value(light_ray.direction());

        if (light_pdf > 0) {
            HitRecord light_rec;
            // Check if the light is visible (no occlusion)
            world.hit(light_ray, Interval(0.001, INFINITY), light_rec);
            if (light_rec.t > 0.9999) {
                Color light_emission = light_rec.mat->emitted(light_ray, light_rec, light_rec.u, light_rec.v, light_rec.p);
                if (light_emission.length2() > 0) {
                    double scattering_pdf = rec.mat->scattering_pdf(r, rec, light_ray);

                    // BALANCE HEURISTIC
                    // double weight_nee = light_pdf / (light_pdf + scattering_pdf);
                    // colorFromScatter += weight_nee * scatterRecord.attenuation * scattering_pdf * light_emission / light_pdf;

                    // POWER HEURISTIC (beta = 2)
                    double light_pdf_2 = light_pdf * light_pdf;
                    double scattering_pdf_2 = scattering_pdf * scattering_pdf;
                    double weight_nee = light_pdf_2 / (light_pdf_2 +scattering_pdf_2);
                    colorFromScatter += weight_nee * scatterRecord.attenuation * scattering_pdf * light_emission / light_pdf;
                }
            }
        }

        // Standard path tracing
        const auto scattered = Ray(rec.p, scatterRecord.pdf_ptr->generate());
        const auto brdf_pdf = scatterRecord.pdf_ptr->value(scattered.direction());

        if (brdf_pdf > 0) {
            const double scatteringPdf = rec.mat->scattering_pdf(r, rec, scattered);
            const Color sampleColor = rayColor(scattered, depth - 1, world, lights);


            auto light_ptr_for_weight = make_shared<HittablePDF>(lights, rec.p);
            double light_pdf_for_this_direction = light_ptr_for_weight->value(scattered.direction());

            // BALANCE HEURISTIC
            // double weight_pt = brdf_pdf / (light_pdf_for_this_direction + brdf_pdf);

            // POWER HEURISTIC (beta = 2)
            double light_pdf_2 = light_pdf_for_this_direction * light_pdf_for_this_direction;
            double brdf_pdf_2 = brdf_pdf * brdf_pdf;
            double weight_pt = brdf_pdf_2 / (light_pdf_2 + brdf_pdf_2);

            colorFromScatter += weight_pt * (scatterRecord.attenuation * scatteringPdf * sampleColor) / brdf_pdf;
        }
    } else {
        // Standard path tracing
        const auto light_ptr = make_shared<HittablePDF>(lights, rec.p);
        // const MixturePDF p(light_ptr, scatterRecord.pdf_ptr);

        const MixturePDF p(light_ptr, scatterRecord.pdf_ptr);

        const auto scattered = Ray(rec.p, p.generate());
        const auto pdfValue = p.value(scattered.direction());

        const double scatteringPdf = rec.mat->scattering_pdf(r, rec, scattered);

        const Color sampleColor = rayColor(scattered, depth - 1, world, lights);
        colorFromScatter += (scatterRecord.attenuation * scatteringPdf * sampleColor) / pdfValue;

        return color_from_emission + colorFromScatter;
    }
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
    aggregator->traverse();

    while (aggregator-> has_next()) {
        Sample sample = aggregator->next();

        Ray r = get_ray(sample.x, sample.y);

        size_t retries = 0;
        Color color;

        do {
            color = rayColor(r, static_cast<int>(maxDepth), world, lights);
        } while (color.near_zero() && ++retries < 20);
        aggregator->insert_contribution(color);
    }

    const Color pixel_color = aggregator->aggregate();

    persist_color_to_data(row, column, pixel_color);

    return aggregator;
}

#ifdef FUNCTION_PARSING
FunctionCamera::FunctionCamera(shared_ptr<Function> function): ForwardParallelCamera(), function(function) {}


std::shared_ptr<SampleAggregator> FunctionCamera::renderPixel(const Hittable &world, const Hittable &lights, size_t row, size_t column) {
    randomSeed(combine(seed, row, column));

    const auto aggregator = samplerAggregator->create();
    aggregator->sampleFrom(pixelSamplerFactory, static_cast<double>(column), static_cast<double>(row));
    aggregator->traverse();

    while (aggregator-> hasNext()) {
        Sample sample = aggregator->next();

        double value = function->compute(sample.dx, sample.dy);
        
        const Color color(value, value, value);
        aggregator->insertContribution(color);
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
