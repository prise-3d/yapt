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

    for (int j = 0; j < image_height; j++) {
        std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
        for (int i = 0; i < image_width; i++) {
            Color pixel_color(0, 0, 0);
            for (int s_j = 0; s_j < sqrt_spp; s_j++) {
                for (int s_i = 0; s_i < sqrt_spp; s_i++) {
                    Ray r = get_ray(i, j, s_i, s_j);
                    pixel_color += rayColor(r, max_depth, world, lights);
                }
            }
            pixel_color *= pixel_samples_scale;

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
    image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;

    sqrt_spp = int(sqrt(samples_per_pixel));
    pixel_samples_scale = 1.0 / (sqrt_spp * sqrt_spp);
    recip_sqrt_spp = 1.0 / sqrt_spp;

    center = lookfrom;

    // Determine viewport dimensions.
    auto theta = degrees_to_radians(vfov);
    auto h = tan(theta/2);
    auto viewport_height = 2 * h * focus_dist;
    auto viewport_width = viewport_height * (double(image_width)/image_height);

    // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
    w = unit_vector(lookfrom - lookat);
    u = unit_vector(cross(vup, w));
    v = cross(w, u);

    // Calculate the vectors across the horizontal and down the vertical viewport edges.
    Vec3 viewport_u = viewport_width * u;    // Vector across viewport horizontal edge
    Vec3 viewport_v = viewport_height * -v;  // Vector down viewport vertical edge

    // Calculate the horizontal and vertical delta vectors from pixel to pixel.
    pixel_delta_u = viewport_u / image_width;
    pixel_delta_v = viewport_v / image_height;

    // Calculate the location of the upper left pixel.
    auto viewport_upper_left = center - (focus_dist * w) - viewport_u/2 - viewport_v/2;
    pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

    // Calculate the camera defocus disk basis vectors.
    auto defocus_radius = focus_dist * tan(degrees_to_radians(defocus_angle / 2));
    defocus_disk_u = u * defocus_radius;
    defocus_disk_v = v * defocus_radius;

    imageData.data = std::vector<uint8_t>(image_width * image_height * 3);
    imageData.width = image_width;
    imageData.height = image_height;
}

Ray Camera::get_ray(int i, int j, int s_i, int s_j) const {
    // Construct a camera ray originating from the defocus disk and directed at a randomly
    // sampled point around the pixel location i, j for stratified sample square s_i, s_j.

    auto offset = sampleSquareStratified(s_i, s_j);
    auto pixel_sample = pixel00_loc
                        + ((i + offset.x()) * pixel_delta_u)
                        + ((j + offset.y()) * pixel_delta_v);

    auto ray_origin = (defocus_angle <= 0) ? center : defocusDiskSample();
    auto ray_direction = pixel_sample - ray_origin;

    return Ray(ray_origin, ray_direction);
}

Vec3 Camera::sampleSquareStratified(int s_i, int s_j) const {
    // Returns the vector to a random point in the square sub-pixel specified by grid
    // indices s_i and s_j, for an idealized unit square pixel [-.5,-.5] to [+.5,+.5].

    auto px = ((s_i + randomDouble()) * recip_sqrt_spp) - 0.5;
    auto py = ((s_j + randomDouble()) * recip_sqrt_spp) - 0.5;

    return {px, py, 0};
}

Vec3 Camera::sample_square() const {
    // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
    return {randomDouble() - 0.5, randomDouble() - 0.5, 0};
}

Vec3 Camera::sampleDisk(double radius) const {
    // Returns a random point in the unit (radius 0.5) disk centered at the origin.
    return radius * random_in_unit_disk();
}

Point3 Camera::defocusDiskSample() const {
    // Returns a random point in the camera defocus disk.
    auto p = random_in_unit_disk();
    return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
}

Color Camera::rayColor(const Ray& r, int depth, const Hittable& world, const Hittable& lights)
const {
    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0)
        return Color(0, 0, 0);

    HitRecord rec;

    // If the ray hits nothing, return the background color.
    if (!world.hit(r, Interval(0.001, infinity), rec))
        return background;

    ScatterRecord srec;
    Color color_from_emission = rec.mat->emitted(r, rec, rec.u, rec.v, rec.p);

    if (!rec.mat->scatter(r, rec, srec))
        return color_from_emission;

    if (srec.skip_pdf) {
        return srec.attenuation * rayColor(srec.skip_pdf_ray, depth - 1, world, lights);
    }

    auto light_ptr = make_shared<HittablePDF>(lights, rec.p);
    MixturePDF p(light_ptr, srec.pdf_ptr);

    Ray scattered = Ray(rec.p, p.generate());
    auto pdf_val = p.value(scattered.direction());

    double scattering_pdf = rec.mat->scattering_pdf(r, rec, scattered);

    Color sample_color = rayColor(scattered, depth - 1, world, lights);
    Color color_from_scatter = (srec.attenuation * scattering_pdf * sample_color) / pdf_val;

    return color_from_emission + color_from_scatter;
}