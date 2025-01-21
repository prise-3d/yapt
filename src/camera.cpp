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
Ray Camera::getRay(double x, double y) const {
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


void ForwardCamera::renderLine(const Hittable &world, const Hittable &lights, size_t j) {
    for (size_t column = 0; column < imageWidth; ++column) {
        renderPixel(world, lights, j, column);
    }
}

inline uint64_t combine(const uint32_t seed, const uint32_t x, const uint32_t y) {
    auto combined = static_cast<uint64_t>(seed);
    combined = (combined << 32) | ((static_cast<uint64_t>(x & 0xFFFF) << 16) | (y & 0xFFFF));
    return combined;
}

std::shared_ptr<SampleAggregator> ForwardCamera::renderPixel(const Hittable &world, const Hittable &lights,
                                                             const size_t row, const size_t column) {
    randomSeed(combine(seed, row, column));

    const auto aggregator = samplerAggregator->create();
    aggregator->sampleFrom(pixelSamplerFactory, static_cast<double>(column), static_cast<double>(row));
    aggregator->traverse();

    while (aggregator-> hasNext()) {
        Sample sample = aggregator->next();

        Ray r = getRay(sample.x, sample.y);

        const Color color = rayColor(r, static_cast<int>(maxDepth), world, lights);
        aggregator->insertContribution(color);
    }

    const Color pixel_color = aggregator->aggregate();

    const size_t idx = 3 * (column + row * imageWidth);

    imageData.data[idx]     = pixel_color.x();  // R
    imageData.data[idx + 1] = pixel_color.y();  // G
    imageData.data[idx + 2] = pixel_color.z();  // B

    return aggregator;
}


void ForwardCamera::render(const Hittable& world, const Hittable& lights) {
    initialize();

    for (int j = 0; j < imageHeight; j++) {
        std::clog << "\rScanlines remaining: " << (imageHeight - j) << ' ' << std::flush;
        renderLine(world, lights, j);
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

    const auto light_ptr = make_shared<HittablePDF>(lights, rec.p);
    const MixturePDF p(light_ptr, scatterRecord.pdf_ptr);

    const auto scattered = Ray(rec.p, p.generate());
    const auto pdfValue = p.value(scattered.direction());

    const double scatteringPdf = rec.mat->scattering_pdf(r, rec, scattered);


    const Color sampleColor = rayColor(scattered, depth - 1, world, lights);
    const Color colorFromScatter = (scatterRecord.attenuation * scatteringPdf * sampleColor) / pdfValue;

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
                renderLine(world, lights, j);
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
    renderPixel(world, lights, pixel_y, pixel_x);
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
std::shared_ptr<SampleAggregator> CartographyCamera::renderPixel(const Hittable &world, const Hittable &lights,
                                                                 const size_t row, const size_t column) {
    std::clog << "Rendering pixel @ " << column << ", " << row << std::endl;
    for (size_t y = 0 ; y < imageHeight ; ++y) {
        const double dy = static_cast<double>(y) / static_cast<double>(imageHeight) - .5;
        for (size_t x = 0 ; x < imageWidth ; ++x) {
            const double dx = static_cast<double>(x) / static_cast<double>(imageWidth) - .5;
            Ray r = getRay(dx + static_cast<double>(column), dy + static_cast<double>(row));

            Color color = rayColor(r, static_cast<int>(maxDepth), world, lights);

            const size_t idx = 3 * (x + y * imageWidth);

            imageData.data[idx]     = color.x();  // R
            imageData.data[idx + 1] = color.y();  // G
            imageData.data[idx + 2] = color.z();  // B
        }
    }

    return nullptr;
}

std::shared_ptr<SampleAggregator> BiasedForwardParallelCamera::renderPixel(
    const Hittable &world, const Hittable &lights, size_t row, size_t column) {
    const auto aggregator = samplerAggregator->create();
    aggregator->sampleFrom(pixelSamplerFactory, static_cast<double>(column), static_cast<double>(row));
    aggregator->traverse();

    while (aggregator-> hasNext()) {
        Sample sample = aggregator->next();

        Ray r = getRay(sample.x, sample.y);

        size_t retries = 0;
        Color color;

        do {
            color = rayColor(r, static_cast<int>(maxDepth), world, lights);
        } while (color.near_zero() && ++retries < 20);
        aggregator->insertContribution(color);
    }

    const Color pixel_color = aggregator->aggregate();

    const size_t idx = 3 * (column + row * imageWidth);

    imageData.data[idx] = pixel_color.x();      // R
    imageData.data[idx + 1] = pixel_color.y();  // G
    imageData.data[idx + 2] = pixel_color.z();  // B

    return aggregator;
}

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

    const size_t idx = 3 * (column + row * imageWidth);

    imageData.data[idx]     = pixel_color.x();  // R
    imageData.data[idx + 1] = pixel_color.y();  // G
    imageData.data[idx + 2] = pixel_color.z();  // B

    return aggregator;
}



