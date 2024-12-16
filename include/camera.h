//
// Created by franck on 09/06/24.
//

#ifndef YAPT_CAMERA_H
#define YAPT_CAMERA_H

#include "yapt.h"
#include "hittable.h"
#include "image_data.h"
#include "sampler.h"
#include "aggregators.h"
#include "path.h"

class Camera {
public:
    virtual ~Camera() = default;

    double aspect_ratio = 1.0;  // Ratio of image width over height
    size_t imageWidth = 100;  // Rendered image width in pixel count
    size_t imageHeight;         // Rendered image height
    size_t maxDepth = 10;   // Maximum number of ray bounces into scene
    shared_ptr<SamplerFactory> pixelSamplerFactory;
    shared_ptr<AggregatorFactory> samplerAggregator;
    Color background;               // Scene background color
    std::size_t numThreads = 0;

    double vfov = 90;              // Vertical view angle (field of view)
    Point3 lookFrom = Point3(0, 0, 0);   // Point camera is looking from
    Point3 lookAt = Point3(0, 0, -1);  // Point camera is looking at
    Vec3 vup = Vec3(0, 1, 0);     // Camera-relative "up" direction

    double defocusAngle = 0;  // Variation angle of rays through each pixel
    double focusDist = 10;    // Distance from camera lookfrom point to plane of perfect focus

    long seed = 0;

    virtual void render(const Hittable &world, const Hittable &lights) = 0;
    shared_ptr<ImageData> data() {return make_shared<ImageData>(imageData);}
    virtual std::shared_ptr<SampleAggregator> renderPixel(const Hittable &world, const Hittable &lights, size_t row,
                                                          size_t column) = 0;

protected:
    Point3 center;           // Camera center
    Point3 pixel00_loc;      // Location of pixel 0, 0
    Vec3 pixel_delta_u;      // Offset to pixel to the right
    Vec3 pixel_delta_v;      // Offset to pixel below
    Vec3 u, v, w;            // Camera frame basis vectors
    Vec3 defocusDiskU;       // Defocus disk horizontal radius
    Vec3 defocusDiskV;       // Defocus disk vertical radius
    ImageData imageData = ImageData();     // image output

    virtual void initialize();
    [[nodiscard]] Point3 defocusDiskSample() const;
    [[nodiscard]] virtual Ray getRay(double x, double y) const;
};

class ForwardCamera: public Camera {
public:
    ~ForwardCamera() override = default;

    void render(const Hittable &world, const Hittable &lights) override;
    virtual void renderLine(const Hittable &world, const Hittable &lights, size_t j);
    virtual std::shared_ptr<SampleAggregator> renderPixel(const Hittable &world, const Hittable &lights, size_t row,
                                                          size_t column) override;

protected:

    [[nodiscard]] virtual Color rayColor(const Ray &r, int depth, const Hittable &world, const Hittable &lights) const;
};

class ForwardParallelCamera: public ForwardCamera {
public:
    void render(const Hittable &world, const Hittable &lights) override;
    int linesPerBatch = 1;
};

class BiasedForwardParallelCamera: public ForwardParallelCamera {
public:
    std::shared_ptr<SampleAggregator> renderPixel(const Hittable &world, const Hittable &lights, size_t row,
                                                  size_t column) override;
};

class TestCamera final : public ForwardParallelCamera {

    [[nodiscard]] Ray getRay(const double x, const double y) const override {
        double ex, ey;
        const double dx = modf(x, &ex);
        const double dy = modf(y, &ey);

        return {Point3(dx, dy, 0), Vec3(0, 0, 0)};
    }

    [[nodiscard]] Color rayColor(const Ray &r, int depth, const Hittable &world, const Hittable &lights) const override {
        if (-r.origin().x() + r.origin().y() > 0) {
            return {0, 0, 0};
        } else return {1, 1, 1};
    }
};

class CartographyCamera final : public ForwardCamera {
public:
    size_t pixel_x;
    size_t pixel_y;

    CartographyCamera(size_t pixel_x, size_t pixel_y);
    void render(const Hittable &world, const Hittable &lights) override;
    std::shared_ptr<SampleAggregator> renderPixel(const Hittable &world, const Hittable &lights, size_t row,
                                                  size_t column) override;

protected:
    void initialize() override;
};
#endif //YAPT_CAMERA_H