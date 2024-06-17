//
// Created by franck on 09/06/24.
//

#ifndef YAPT_CAMERA_H
#define YAPT_CAMERA_H

#include "yapt.h"
#include "hittable.h"
#include "image_data.h"
#include <png.h>

class Camera {
public:
    double aspect_ratio = 1.0;  // Ratio of image width over height
    int image_width = 100;  // Rendered image width in pixel count
    int image_height;         // Rendered image height
    int samples_per_pixel = 10;   // Count of random samples for each pixel
    int max_depth = 10;   // Maximum number of ray bounces into scene
    Color background;               // Scene background color

    double vfov = 90;              // Vertical view angle (field of view)
    Point3 lookfrom = Point3(0, 0, 0);   // Point camera is looking from
    Point3 lookat = Point3(0, 0, -1);  // Point camera is looking at
    Vec3 vup = Vec3(0, 1, 0);     // Camera-relative "up" direction

    double defocus_angle = 0;  // Variation angle of rays through each pixel
    double focus_dist = 10;    // Distance from camera lookfrom point to plane of perfect focus

    virtual void render(const Hittable &world, const Hittable &lights);
    shared_ptr<ImageData> data() {return make_shared<ImageData>(imageData);}

protected:
    double pixel_samples_scale;  // Color scale factor for a sum of pixel samples
    int sqrt_spp;             // Square root of number of samples per pixel
    double recip_sqrt_spp;       // 1 / sqrt_spp
    Point3 center;               // Camera center
    Point3 pixel00_loc;          // Location of pixel 0, 0
    Vec3 pixel_delta_u;        // Offset to pixel to the right
    Vec3 pixel_delta_v;        // Offset to pixel below
    Vec3 u, v, w;              // Camera frame basis vectors
    Vec3 defocus_disk_u;       // Defocus disk horizontal radius
    Vec3 defocus_disk_v;       // Defocus disk vertical radius
    ImageData imageData;       // image output

    virtual void initialize();

    [[nodiscard]] virtual Color rayColor(const Ray &r, int depth, const Hittable &world, const Hittable &lights) const;

    [[nodiscard]] virtual Ray get_ray(int i, int j, int s_i, int s_j) const;

    [[nodiscard]] virtual Vec3 sampleSquareStratified(int s_i, int s_j) const;

    [[nodiscard]] Vec3 sample_square() const;

    [[nodiscard]] Point3 defocusDiskSample() const;

    [[nodiscard]] Vec3 sampleDisk(double radius) const;
};

#endif //YAPT_CAMERA_H
