//
// Created by franck on 09/06/24.
//

#include "yapt.h"
#include "camera.h"
#include "material.h"
#include <vector>

void Camera::render(const Hittable& world, const Hittable& lights) {
    initialize();
    int idx = 0;

    for (int j = 0; j < imageHeight; j++) {
        std::clog << "\rScanlines remaining: " << (imageHeight - j) << ' ' << std::flush;
        for (int i = 0; i < imageWidth; i++) {
            Color pixel_color(0, 0, 0);

            auto sampler = pixelSamplerFactory->create(i, j);
            for (sampler->begin(); sampler->hasNext() ;) {
                Point3 p = sampler->get();
                Ray r = getRay(p.x(), p.y());
                pixel_color += rayColor(r, maxDepth, world, lights);
            }

            pixel_color /= sampler->sampleSize();
            auto r = pixel_color.x();
            auto g = pixel_color.y();
            auto b = pixel_color.z();

            // Apply a linear to gamma transform for gamma 2
            r = linearToGamma(r);
            g = linearToGamma(g);
            b = linearToGamma(b);

            // Translate the [0,1] component values to the byte range [0,255].
            static const Interval intensity(0.000, 0.999);
            int rbyte = int(256 * intensity.clamp(r));
            int gbyte = int(256 * intensity.clamp(g));
            int bbyte = int(256 * intensity.clamp(b));

            imageData.data[idx++] = rbyte;  // R
            imageData.data[idx++] = gbyte;  // G
            imageData.data[idx++] = bbyte;  // B
        }
    }
}

void Camera::initialize() {
    imageHeight = int(imageWidth / aspect_ratio);
    imageHeight = (imageHeight < 1) ? 1 : imageHeight;

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

    imageData.data = std::vector<uint8_t>(imageWidth * imageHeight * 3);
    imageData.width = imageWidth;
    imageData.height = imageHeight;
}

/**
 * Gets a ray from a fractional pixel position in the pixel grid
 * @param x x (fractionnal) coordinate of a pixel. 0 <= x < imageWidth
 * @param y y (fractionnal) coordinate of a pixel. 0 <= y < imageHeight
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

Point3 Camera::defocusDiskSample() const {
    // Returns a random point in the camera defocus disk.
    auto p = random_in_unit_disk();
    return center + (p[0] * defocusDiskU) + (p[1] * defocusDiskV);
}

Color Camera::rayColor(const Ray& r, int depth, const Hittable& world, const Hittable& lights)
const {
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