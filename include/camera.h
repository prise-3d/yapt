//
// Created by franck on 09/06/24.
//

#ifndef YAPT_CAMERA_H
#define YAPT_CAMERA_H

#include "yapt.h"
#include "hittable.h"
#include <png.h>

class Camera {
public:
    double aspect_ratio = 1.0;  // Ratio of image width over height
    int image_width = 100;  // Rendered image width in pixel count
    int samples_per_pixel = 10;   // Count of random samples for each pixel
    int max_depth = 10;   // Maximum number of ray bounces into scene
    Color background;               // Scene background color

    double vfov = 90;              // Vertical view angle (field of view)
    Point3 lookfrom = Point3(0, 0, 0);   // Point camera is looking from
    Point3 lookat = Point3(0, 0, -1);  // Point camera is looking at
    Vec3 vup = Vec3(0, 1, 0);     // Camera-relative "up" direction

    double defocus_angle = 0;  // Variation angle of rays through each pixel
    double focus_dist = 10;    // Distance from camera lookfrom point to plane of perfect focus

    void render(const Hittable &world, const Hittable &lights);

private:
    int image_height;         // Rendered image height
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

    void initialize();

    [[nodiscard]] Color ray_color(const Ray &r, int depth, const Hittable &world, const Hittable &lights) const;

    [[nodiscard]] Ray get_ray(int i, int j, int s_i, int s_j) const;

    [[nodiscard]] Vec3 sampleSquareStratified(int s_i, int s_j) const;

    [[nodiscard]] Vec3 sample_square() const;

    [[nodiscard]] Point3 defocusDiskSample() const;

    [[nodiscard]] Vec3 sample_disk(double radius) const;
};

inline bool write_png(const std::string &file_name, const std::vector<uint8_t> &image_data, int width, int height) {
    FILE *fp = fopen(file_name.c_str(), "wb");
    if (!fp) {
        std::cerr << "Erreur d'ouverture du fichier" << std::endl;
        return false;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr) {
        std::cerr << "Erreur de création de la structure png" << std::endl;
        fclose(fp);
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        std::cerr << "Erreur de création de la structure d'information png" << std::endl;
        png_destroy_write_struct(&png_ptr, nullptr);
        fclose(fp);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        std::cerr << "Erreur lors de l'écriture de l'image PNG" << std::endl;
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return false;
    }

    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, width, height,
                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    std::vector<png_bytep> row_pointers(height);
    for (int y = 0; y < height; y++) {
        row_pointers[y] = const_cast<png_bytep>(&image_data[y * width * 3]);
    }

    png_set_rows(png_ptr, info_ptr, row_pointers.data());
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);

    return true;
}

#endif //YAPT_CAMERA_H
