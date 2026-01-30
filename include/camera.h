#ifndef YAPT_CAMERA_H
#define YAPT_CAMERA_H

#include "yapt.h"
#include "hittable.h"
#include "image_data.h"
#include "sampler.h"
#include "aggregators.h"
#include "sampling_strategy.h"
#include "ray_evaluator.h"
#include "scene.h"

#ifdef FUNCTION_PARSING
    #include "functions.h"
#endif

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
    shared_ptr<SamplingStrategy> samplingStrategy;

    virtual void render(const Scene &scene) = 0;
    shared_ptr<ImageData> data() {return make_shared<ImageData>(imageData);}
    virtual std::shared_ptr<SampleAggregator> render_pixel(const Scene &scene, size_t row,
                                                          size_t column) = 0;

    shared_ptr<RayEvaluator> scattering_strategy;

    virtual void initialize();

protected:
    Point3 center;           // Camera center
    Point3 pixel00_loc;      // Location of pixel 0, 0
    Vec3 pixel_delta_u;      // Offset to pixel to the right
    Vec3 pixel_delta_v;      // Offset to pixel below
    Vec3 u, v, w;            // Camera frame basis vectors
    Vec3 defocusDiskU;       // Defocus disk horizontal radius
    Vec3 defocusDiskV;       // Defocus disk vertical radius
    ImageData imageData = ImageData();     // image output


    [[nodiscard]] Point3 defocusDiskSample() const;
    [[nodiscard]] virtual Ray get_ray(double x, double y) const;
};

class ForwardCamera: public Camera {
public:
    ~ForwardCamera() override = default;

    void render(const Scene &scene) override;
    virtual void render_line(const Scene &scene, size_t j);
    void persist_color_to_data(size_t row, size_t column, Color pixel_color);

    std::shared_ptr<SampleAggregator> render_pixel(const Scene &scene, size_t row,
                                                          size_t column) override;
};

class ForwardParallelCamera: public ForwardCamera {
public:
    void render(const Scene &scene) override;
    int linesPerBatch = 1;
};

class BiasedForwardParallelCamera: public ForwardParallelCamera {
public:
    std::shared_ptr<SampleAggregator> render_pixel(const Scene &scene, size_t row,
                                                  size_t column) override;
};

class TestCamera final : public ForwardParallelCamera {
    [[nodiscard]] Ray get_ray(const double x, const double y) const override;
    // [[nodiscard]] Color rayColor(const Ray &r, int depth, const Hittable &world, const Hittable &lights) const override;
};

class CartographyCamera final : public ForwardCamera {
public:
    size_t pixel_x;
    size_t pixel_y;

    CartographyCamera(size_t pixel_x, size_t pixel_y);
    void render(const Scene &scene) override;
    std::shared_ptr<SampleAggregator> render_pixel(const Scene &scene, size_t row,
                                                  size_t column) override;

    void initialize() override;

};

class FunctionCamera final : public ForwardParallelCamera {
    public:
#ifdef FUNCTION_PARSING
    FunctionCamera(shared_ptr<Function> function);
#endif
    std::shared_ptr<SampleAggregator> render_pixel(const Scene &scene, size_t row, size_t column) override;


protected:
#ifdef FUNCTION_PARSING
    shared_ptr<Function> function;
#endif
};

class SinglePixelCamera: public ForwardCamera {

public:
    SinglePixelCamera(size_t pixel_x, size_t pixel_y);
    void render(const Scene &scene) override;

    size_t pixel_x;
    size_t pixel_y;
};

#endif //YAPT_CAMERA_H