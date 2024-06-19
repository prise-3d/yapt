//
// Created by franck on 09/06/24.
//

#ifndef YAPT_CAMERA_H
#define YAPT_CAMERA_H

#include "yapt.h"
#include "hittable.h"
#include "image_data.h"
#include <png.h>
#include "sampler.h"

class Camera {
public:
    double aspect_ratio = 1.0;  // Ratio of image width over height
    int imageWidth = 100;  // Rendered image width in pixel count
    int imageHeight;         // Rendered image height
    int maxDepth = 10;   // Maximum number of ray bounces into scene
    shared_ptr<PixelSamplerFactory> pixelSamplerFactory;
    Color background;               // Scene background color

    double vfov = 90;              // Vertical view angle (field of view)
    Point3 lookFrom = Point3(0, 0, 0);   // Point camera is looking from
    Point3 lookAt = Point3(0, 0, -1);  // Point camera is looking at
    Vec3 vup = Vec3(0, 1, 0);     // Camera-relative "up" direction

    double defocusAngle = 0;  // Variation angle of rays through each pixel
    double focusDist = 10;    // Distance from camera lookfrom point to plane of perfect focus

    virtual void render(const Hittable &world, const Hittable &lights);
    virtual void renderLine(const Hittable &world, const Hittable &lights, int j);
    virtual void parallelRender(const Hittable &world, const Hittable &lights);

    shared_ptr<ImageData> data() {return make_shared<ImageData>(imageData);}

protected:
    Point3 center;               // Camera center
    Point3 pixel00_loc;          // Location of pixel 0, 0
    Vec3 pixel_delta_u;        // Offset to pixel to the right
    Vec3 pixel_delta_v;        // Offset to pixel below
    Vec3 u, v, w;              // Camera frame basis vectors
    Vec3 defocusDiskU;       // Defocus disk horizontal radius
    Vec3 defocusDiskV;       // Defocus disk vertical radius
    ImageData imageData;       // image output

    virtual void initialize();

    [[nodiscard]] virtual Color rayColor(const Ray &r, int depth, const Hittable &world, const Hittable &lights) const;

    [[nodiscard]] Point3 defocusDiskSample() const;

    Ray getRay(double x, double y) const;
};

#endif //YAPT_CAMERA_H
