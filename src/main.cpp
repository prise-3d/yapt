//
// Created by franck on 07/06/24.
//

#include "yapt.h"
#include "hittable_list.h"
#include "sphere.h"
#include "camera.h"
#include "material.h"
#include <chrono>
#include "texture.h"
#include "quad.h"
#include "image_exporter.h"
#include "bvh.h"
#include <iomanip>


void simple_light() {
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
    exporter.write("/home/franck/out.png");
}

void final () {
    HittableList world;

    auto red   = make_shared<Lambertian>(Color(.65, .05, .05));
    auto white = make_shared<Lambertian>(Color(.73, .73, .73));
    auto green = make_shared<Lambertian>(Color(.12, .45, .15));
    auto light = make_shared<DiffuseLight>(Color(15, 15, 15));

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
    lights.add(make_shared<Sphere>(Point3(190, 90, 190), 90, m));

    Camera cam;

    cam.aspect_ratio      = 1.0;
    cam.imageWidth       = 900;
    cam.maxDepth         = 25;
    cam.background        = Color(0, 0, 0);

    cam.pixelSamplerFactory = make_shared<StratifiedPixelSamplerFactory>(10);

    cam.vfov     = 40;
    cam.lookFrom = Point3(278, 278, -800);
    cam.lookAt   = Point3(278, 278, 0);
    cam.vup      = Vec3(0, 1, 0);

    cam.defocusAngle = 0;

    cam.parallelRender(world, lights);

    PNGImageExporter exporter(cam.data());
    exporter.write("/home/franck/out.png");
}

#define PI_N 1000000000
#define PI_N_THREADS 20

void compute_pi() {

    int n_inside = 0;
    for (int i = 0 ; i < PI_N ; i++) {
        double x = randomDouble();
        double y = randomDouble();
        if (x * x + y * y < 1) n_inside++;
    }
    std::cout << 4 * double(n_inside) / double(PI_N) << std::endl;
}

#include <random>
#include <thread>
#include <vector>
#include <atomic>

struct alignas(64) AlignedInt {
    std::atomic<int> value{0};
};

void compute_some_pi(AlignedInt* count) {
    thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (int i = 0; i < PI_N / PI_N_THREADS; i++) {
        double x = dist(rng);
        double y = dist(rng);
        if (x * x + y * y < 1) count->value++;
    }
}

void compute_pi_several_threads() {
    AlignedInt n_inside[PI_N_THREADS];

    std::vector<std::thread> threads;
    for (int i = 0; i < PI_N_THREADS; ++i) {
        threads.emplace_back(compute_some_pi, &n_inside[i]);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    int inside = 0;
    for (int i = 0; i < PI_N_THREADS; i++) {
        inside += n_inside[i].value.load();
    }

    std::cout << 4 * double(inside) / double(PI_N) << std::endl;
}


int main() {

    auto start = std::chrono::high_resolution_clock::now();

    switch(1) {
        case 1: final(); break;
        case 2: simple_light(); break;
        case 3: compute_pi(); break;
        case 4: compute_pi_several_threads(); break;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
    std::clog << "Rendering duration: " << double(duration.count()) / 1000. << " s" << std::endl;
}