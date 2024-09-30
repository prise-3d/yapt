
//
// Created by franck on 09/06/24.
//

#include "yapt.h"
#include "camera.h"
#include "material.h"
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <condition_variable>
#include <memory>

void Camera::renderLine(const Hittable &world, const Hittable &lights, int j) {
    for (int column = 0; column < imageWidth; ++column) {
        renderPixel(world, lights, j, column);
    }
}

void Camera::renderPixel(const Hittable &world, const Hittable &lights, int row, int column) {
    auto aggregator = samplerAggregator->create();
    aggregator->sampleFrom(pixelSamplerFactory, column, row);
    aggregator->traverse();

    while (aggregator-> hasNext()) {
        Sample sample = aggregator->next();

        Vec3 color(0, 0, 0);
        Ray r = getRay(sample.x, sample.y);
        color = rayColor(r, maxDepth, world, lights);
        aggregator->insertContribution(color);
    }

    Color pixel_color = aggregator->aggregate();

    size_t idx = 3 * (column + row * imageWidth);

    imageData.data[idx++] = pixel_color.x();  // R
    imageData.data[idx++] = pixel_color.y();  // G
    imageData.data[idx]   = pixel_color.z();  // B
}


void Camera::render(const Hittable& world, const Hittable& lights) {
    initialize();

    for (int j = 0; j < imageHeight; j++) {
        std::clog << "\rScanlines remaining: " << (imageHeight - j) << ' ' << std::flush;
        renderLine(world, lights, j);
    }
}

void Camera::initialize() {
    imageHeight = int(imageWidth / aspect_ratio);
    imageHeight = (imageHeight < 1) ? 1 : imageHeight;
    imageData.data = std::vector<double>(imageWidth * imageHeight * 3);
    center = lookFrom;

    // Determine viewport dimensions.
    auto theta = degrees_to_radians(vfov);
    auto h = tan(theta/2);
    auto viewportHeight = 2 * h * focusDist;
    auto viewportWidth = viewportHeight * (double(imageWidth) / imageHeight);

    // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
    w = unit_vector(lookFrom - lookAt);
    u = unit_vector(cross(vup, w));
    v = cross(w, u);

    // Calculate the vectors across the horizontal and down the vertical viewport edges.
    Vec3 viewportU = viewportWidth * u;    // Vector across viewport horizontal edge
    Vec3 viewportV = viewportHeight * -v;  // Vector down viewport vertical edge

    // Calculate the horizontal and vertical delta vectors from pixel to pixel.
    pixel_delta_u = viewportU / imageWidth;
    pixel_delta_v = viewportV / imageHeight;

    // Calculate the location of the upper left pixel.
    auto viewportUpperLeft = center - (focusDist * w) - viewportU / 2 - viewportV / 2;
    pixel00_loc = viewportUpperLeft + 0.5 * (pixel_delta_u + pixel_delta_v);

    // Calculate the camera defocus disk basis vectors.
    auto defocusRadius = focusDist * tan(degrees_to_radians(defocusAngle / 2));
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

Color Camera::rayColor(const Ray& r, int depth, const Hittable& world, const Hittable& lights) const {
    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0)
        return {0, 0, 0};

    HitRecord rec;
    // If the ray hits nothing, return the background color.
    if (!world.hit(r, Interval(0.001, infinity), rec))
        return background;

    ScatterRecord scatterRecord;
    Color color_from_emission = rec.mat->emitted(r, rec, rec.u, rec.v, rec.p);

    if (!rec.mat->scatter(r, rec, scatterRecord))
        return color_from_emission;

    if (scatterRecord.skip_pdf) {
        return scatterRecord.attenuation * rayColor(scatterRecord.skip_pdf_ray, depth - 1, world, lights);
    }

    auto light_ptr = make_shared<HittablePDF>(lights, rec.p);
    MixturePDF p(light_ptr, scatterRecord.pdf_ptr);

    Ray scattered = Ray(rec.p, p.generate());
    auto pdfValue = p.value(scattered.direction());

    double scatteringPdf = rec.mat->scattering_pdf(r, rec, scattered);

    Color sampleColor = rayColor(scattered, depth - 1, world, lights);
    Color colorFromScatter = (scatterRecord.attenuation * scatteringPdf * sampleColor) / pdfValue;

    return color_from_emission + colorFromScatter;
}

void ParallelCamera::render(const Hittable &world, const Hittable &lights) {
    initialize();

    std::mutex queue_mutex;
    std::condition_variable queue_cv;

    // Available threads
    if (numThreads <= 0) numThreads = std::thread::hardware_concurrency();

    std::clog << "rendering using " << numThreads << " threads" << std::endl;
    std::vector<std::thread> threads(numThreads);

    std::queue<std::pair<int, int>> taskQueue;

    for (int start_j = 0 ; start_j < imageHeight ; start_j += linesPerBatch) {
        taskQueue.emplace(start_j, std::min(start_j + linesPerBatch - 1, imageHeight-1));
    }
    std::clog << std::endl;

    // Here we declare a function to process a task (render a few lines of the image)
    auto processSegment = [&]() {
        auto now = std::chrono::high_resolution_clock::now();
        auto seed = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        randomSeed(seed);
        while (true) {
            std::pair<int, int> task;
            unsigned long remainingTasks;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                if (taskQueue.empty()) {
                    break;  // All the tasks are done!
                }
                task = taskQueue.front();
                taskQueue.pop();
                remainingTasks = taskQueue.size();
            }

            std::clog << "\rTasks remaining: " << remainingTasks << "   " << std::flush;

            int start_j = task.first;
            int end_j = task.second;

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

CartographyCamera::CartographyCamera(size_t pixel_x, size_t pixel_y): pixel_x(pixel_x), pixel_y(pixel_y) {}

void CartographyCamera::render(const Hittable &world, const Hittable &lights) {
    initialize();
    renderPixel(world, lights, pixel_y, pixel_x);
}

void CartographyCamera::initialize() {
    Camera::initialize();
}

/**
 * Render a pixel cartography. We assume for now that the pixel is uniformly sampled
 * @param world
 * @param lights
 * @param row
 * @param column
 */
void CartographyCamera::renderPixel(const Hittable &world, const Hittable &lights, int row, int column) {
    std::clog << "Rendering pixel @ " << column << ", " << row << std::endl;
    for (size_t y = 0 ; y < imageHeight ; ++y) {
        double dy = (double)y / imageHeight - .5;
        for (size_t x = 0 ; x < imageWidth ; ++x) {
            double dx = (double)x / imageWidth - .5;
            Ray r = getRay(dx + column, dy + row);
            Color color = rayColor(r, maxDepth, world, lights);

            size_t idx = 3 * (x + y * imageWidth);

            imageData.data[idx]     = color.x();  // R
            imageData.data[idx + 1] = color.y();  // G
            imageData.data[idx + 2] = color.z();  // B
        }
    }
}


