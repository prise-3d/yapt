//
// Created by franck on 07/06/24.
//

#include "bvh.h"
#include "yapt.h"
#include "hittable_list.h"
#include "sphere.h"
#include "camera.h"
#include "material.h"
#include <chrono>
#include "texture.h"
#include "rtw_stb_image.h"
#include "quad.h"
#include "constant_medium.h"
#include "image_exporter.h"
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
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = Color(0, 0, 0);

    cam.vfov     = 20;
    cam.lookfrom = Point3(26, 3, 6);
    cam.lookat   = Point3(0, 2, 0);
    cam.vup      = Vec3(0, 1, 0);

    cam.defocus_angle = 0;

    cam.render(world, lights);
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

    // Light Sources
    HittableList lights;
    auto m = shared_ptr<Material>();
    lights.add(make_shared<Quad>(Point3(343, 554, 332), Vec3(-130, 0, 0), Vec3(0, 0, -105), m));
    lights.add(make_shared<Sphere>(Point3(190, 90, 190), 90, m));

    Camera cam;

    cam.aspect_ratio      = 1.0;
    cam.image_width       = 600;
    cam.samples_per_pixel = 25;
    cam.max_depth         = 25;
    cam.background        = Color(0, 0, 0);

    cam.vfov     = 40;
    cam.lookfrom = Point3(278, 278, -800);
    cam.lookat   = Point3(278, 278, 0);
    cam.vup      = Vec3(0, 1, 0);

    cam.defocus_angle = 0;

    cam.render(world, lights);

    PNGImageExporter exporter(cam.data());
    exporter.write("/home/franck/out.png");
}

int main() {

    auto start = std::chrono::high_resolution_clock::now();

    switch(1) {
        case 1: final(); break;
        case 2: simple_light(); break;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
    std::clog << "Rendering duration: " << double(duration.count()) / 1000. << " s" << std::endl;
}