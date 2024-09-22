//
// Created by franck on 22/06/24.
//

#include "yapt.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"
#include "triangle.h"
#include "quad.h"
#include "image_exporter.h"
#include "camera.h"
#include "bvh.h"

void simple_light(std::string path) {
    HittableList world;
    HittableList lights;

    auto pertext = make_shared<NoiseTexture>(4);
    world.add(make_shared<Sphere>(Point3(0, -1000, 0), 1000, make_shared<Lambertian>(pertext)));
    world.add(make_shared<Sphere>(Point3(0, 2, 0), 2, make_shared<Lambertian>(pertext)));

    auto difflight = make_shared<DiffuseLight>(Color(4, 4, 4));
    auto the_light = make_shared<Quad>(Point3(3, 1, -2), Vec3(2, 0, 0), Vec3(0, 2, 0), difflight);
    world.add(the_light);
    lights.add(the_light);

    Camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.imageWidth       = 1600;
    cam.maxDepth         = 30;
    cam.background        = Color(0, 0, 0);

    cam.vfov     = 20;
    cam.lookFrom = Point3(26, 3, 6);
    cam.lookAt   = Point3(0, 2, 0);
    cam.vup      = Vec3(0, 1, 0);

    cam.pixelSamplerFactory = make_shared<StratifiedPixelSamplerFactory>(10);

    cam.defocusAngle = 0;

    cam.render(world, lights);

    PNGImageExporter exporter(cam.data());
    exporter.write(path);
}


void cornellBox (std::string path, std::shared_ptr<PixelSamplerFactory> pixelSamplerFactory, std::shared_ptr<AggregatorFactory> aggregatorFactory) {
    HittableList world;

    auto red   = make_shared<Lambertian>(Color(.65, .05, .05));
    auto white = make_shared<Lambertian>(Color(.73, .73, .73));
    auto green = make_shared<Lambertian>(Color(.12, .45, .15));
    auto light = make_shared<DiffuseLight>(Color(24, 24, 24));

    // Cornell box sides
    world.add(make_shared<Quad>(Point3(555, 0, 0), Vec3(0, 0, 555), Vec3(0, 555, 0), green));
    world.add(make_shared<Quad>(Point3(0, 0, 555), Vec3(0, 0, -555), Vec3(0, 555, 0), red));
    world.add(make_shared<Quad>(Point3(0, 555, 0), Vec3(555, 0, 0), Vec3(0, 0, 555), white));
    world.add(make_shared<Quad>(Point3(0, 0, 555), Vec3(555, 0, 0), Vec3(0, 0, -555), white));
    world.add(make_shared<Quad>(Point3(555, 0, 555), Vec3(-555, 0, 0), Vec3(0, 555, 0), white));

    // Light
    world.add(make_shared<Quad>(Point3(213, 554, 227), Vec3(130, 0, 0), Vec3(0, 0, 105), light));

    // Box
    shared_ptr<Hittable> box1 = box(Point3(0, 0, 0), Point3(165, 330, 165), white);
    box1 = make_shared<RotateY>(box1, 15);
    box1 = make_shared<Translate>(box1, Vec3(265, 0, 295));
    world.add(box1);

    // Glass Sphere
    auto glass = make_shared<Dielectric>(1.5);
    world.add(make_shared<Sphere>(Point3(190, 90, 190), 90, glass));

    world = HittableList(make_shared<BVHNode>(world));

    // Light Sources
    HittableList lights;
    auto m = shared_ptr<Material>();
    lights.add(make_shared<Quad>(Point3(343, 554, 332), Vec3(-130, 0, 0), Vec3(0, 0, -105), m));
    lights.add(make_shared<Quad>(Point3(213, 554, 227), Vec3(30, 0, 0), Vec3(0, 0, 30), light));

    Camera cam;

    cam.aspect_ratio      = 1.0;
    cam.imageWidth       = 900;
    cam.maxDepth         = 25;
    cam.background        = Color(0, 0, 0);

//    cam.pixelSamplerFactory = make_shared<StratifiedPixelSamplerFactory>(256);
//    cam.pixelSamplerFactory = make_shared<TrivialPixelSamplerFactory>(10000);
//    cam.pixelSamplerFactory = make_shared<PPPPixelSamplerFactory>(500,.999);
    cam.pixelSamplerFactory = pixelSamplerFactory;
    cam.samplerAggregator = aggregatorFactory;
    cam.vfov     = 40;
    cam.lookFrom = Point3(278, 278, -800);
    cam.lookAt   = Point3(278, 278, 0);
    cam.vup      = Vec3(0, 1, 0);

    cam.defocusAngle = 0;

    cam.parallelRender(world, lights);
//    cam.render(world, lights);

    PNGImageExporter exporter(cam.data());
    exporter.write(path);
}

void triangle(std::string path) {
    HittableList world;
    HittableList lights;

    auto pertext = make_shared<NoiseTexture>(4);
    world.add(make_shared<Sphere>(Point3(0, -1000, 0), 1000, make_shared<Lambertian>(pertext)));
    world.add(make_shared<Sphere>(Point3(0, 2, 0), 2, make_shared<Lambertian>(pertext)));
    world.add(make_shared<Triangle>(Point3(-1,5,-1), Vec3(-4,0,-2),Vec3(0,2,0),  make_shared<Lambertian>(pertext)));


    auto difflight = make_shared<DiffuseLight>(Color(4, 4, 4));
//    auto the_light = make_shared<Triangle>(Point3(4, 1, -1),  Point3(4,1,3), Point3(4,6,-1), difflight);
    auto the_light = make_shared<Triangle>(Point3(4, 1, -1),  Point3(0,0,3), Point3(0,5,0), difflight);

    world.add(the_light);
    lights.add(the_light);

    Camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.imageWidth       = 1600;
    cam.maxDepth         = 30;
    cam.background        = Color(0.3, 0.3, 0.3);

    cam.vfov     = 20;
    cam.lookFrom = Point3(-5, 3, 30);
    cam.lookAt   = Point3(0, 2, 0);
    cam.vup      = Vec3(0, 1, 0);

    cam.pixelSamplerFactory = make_shared<StratifiedPixelSamplerFactory>(6);

    cam.defocusAngle = 0;

    cam.parallelRender(world, lights);

    PNGImageExporter exporter(cam.data());
    exporter.write(path);
}

void single_triangle(std::string path) {
    auto green = make_shared<Lambertian>(Color(.12, .45, .15));
    HittableList world;

    world.add(make_shared<Triangle>(Point3(-1,-1,-1), Point3(0,2,0), Point3(0,0,-2), green));

    Camera cam;

    cam.aspect_ratio      = 1.0;
    cam.imageWidth       = 900;
    cam.maxDepth         = 25;
    cam.background        = Color(0, 0, 0);

    cam.pixelSamplerFactory = make_shared<StratifiedPixelSamplerFactory>(5);

    cam.vfov     = 40;
    cam.lookFrom = Point3(4, 4, 5);
    cam.lookAt   = Point3(0, 0, 0);
    cam.vup      = Vec3(0, 1, 0);

    cam.defocusAngle = 0;

    auto difflight = make_shared<DiffuseLight>(Color(4, 4, 4));
    HittableList lights;
    lights.add(make_shared<Sphere>(Point3(1.2, 3, .5), 90, difflight));

    world.add(make_shared<Sphere>(Point3(1.2, 3, .5), 90, difflight));

    cam.parallelRender(world, lights);

    PNGImageExporter exporter(cam.data());
    exporter.write(path);
}

void original(std::string path) {
    HittableList world;

    auto groundMaterial = make_shared<Lambertian>(Color(.5, .5, .5));
    world.add(make_shared<Sphere>(Point3(0,-1000,0), 1000, groundMaterial));

    auto material3 = make_shared<Metal>(Color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<Sphere>(Point3(4, 1, 0), 1.0, material3));

    world = HittableList(make_shared<BVHNode>(world));

    Camera cam;

    cam.aspect_ratio      = 16.0/9;
    cam.imageWidth       = 1600;
    cam.maxDepth         = 25;
    cam.background        = Color(0.7, 0.7, 0.95);

    cam.pixelSamplerFactory = make_shared<StratifiedPixelSamplerFactory>(23);

    cam.vfov     = 20;
    cam.lookFrom = Point3(13,2,3);
    cam.lookAt   = Point3(0,0,0);
    cam.vup      = Vec3(0, 1, 0);

    cam.defocusAngle = 0.2;
    cam.focusDist = 10.0;

    HittableList lights;
    auto lightMat = make_shared<DiffuseLight>(Color(6.2, 6.2, 6.2));

    lights.add(make_shared<Sphere>(Point3(0, 28, 0), 5, lightMat));
    world.add(make_shared<Sphere>(Point3(0, 28, 0), 5, lightMat));

    cam.parallelRender(world, lights);

    PNGImageExporter exporter(cam.data());
    exporter.write(path);
}
