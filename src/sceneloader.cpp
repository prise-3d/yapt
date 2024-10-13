//
// Created by franck on 26/09/24.
//
#include "yapt.h"
#include "sceneloader.h"
#include "camera.h"
#include "hittable.h"
#include "material.h"
#include "quad.h"
#include "sphere.h"
#include "bvh.h"
#include <regex>
#include <fstream>

Vec3 vectorMatch(std::smatch &matches, size_t index) {
    double r = std::stod(matches[index]);
    double g = std::stod(matches[index + 1]);
    double b = std::stod(matches[index + 2]);
    return {r, g, b};
}

void YaptSceneLoader::load(std::string path, shared_ptr <HittableList> scene, shared_ptr <HittableList> lights, shared_ptr <Camera> camera) {

    std::ifstream file(path);

    if (!file.is_open()) {
        std::cerr << "Scene file not found: " << path << std::endl;
        return;
    }

    std::string line;

    while (std::getline(file, line)) {
        std::regex material(R"(material\s*:\s*(\w+?))");
        std::regex object(R"(object\s*:\s*(\w+?))");
        std::regex sceneexp(R"(scene\s*:\s*(\w+?))");
        std::regex lightsexp(R"(lights\s*:\s*(\w+?))");
        std::smatch matches;

        if (std::regex_match(line, matches, material)) {
            std::clog << "found: Material " << matches[1] << std::endl;
            materials[matches[1]] = loadMaterial(file);
        } else
        if (std::regex_match(line, matches, object)) {
            std::clog << "found: Object " << matches[1] << std::endl;
            hittables[matches[1]] = loadHittable(file);
        } else
        if (std::regex_match(line, matches, sceneexp)) {
            std::clog << "found: Scene " << matches[1] << std::endl;
            std::clog << "hittables found so far..." << std::endl;
            for (const auto& pair : hittables) {
                std::cout << " - key: " << pair.first << ", value: " << pair.second << std::endl;
            }
            std::clog << "materials found so far..." << std::endl;
            for (const auto& pair : materials) {
                std::cout << " - key: " << pair.first << ", value: " << pair.second << std::endl;
            }

            scene->add(loadScene(file));
        } else
        if (std::regex_match(line, matches, lightsexp)) {
            std::clog << "found: Lights " << matches[1] << std::endl;
            scene->add(loadLights(file, lights));
        }

    }
}

shared_ptr<Material> YaptSceneLoader::loadMaterial(std::ifstream &file) {
    std::string line;
    std::getline(file, line);
    std::regex lambertian(R"(lambertian\s*=\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+))");
    std::regex difflight(R"(difflight\s*=\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+))");
    std::regex metal(R"(metal\s*=\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+)(?:\s*,\s*([0-9]*\.?[0-9]+))?)");
    std::regex dielectric(R"(dielectric\s*=\s*([0-9]*\.?[0-9]+))");
    std::regex isotropic(R"(isotropic\s*=\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+))");
    std::regex composite(R"(composite\s*=\s*([\w]+)\s*-\s*([\w]+)\s*-\s*(-?[0-9]+\.?[0-9]*))");
    std::smatch matches;

    if (std::regex_match(line, matches, lambertian)) {
        Color color = vectorMatch(matches, 1);
        std::clog << "found: Lambertian " << color << std::endl;
        return make_shared<Lambertian>(color);
    } else
    if (std::regex_match(line, matches, difflight)) {
        Color color = vectorMatch(matches, 1);
        std::clog << "found: DiffuseLight " << color << std::endl;
        return make_shared<DiffuseLight>(color);
    } else
    if (std::regex_match(line, matches, metal)) {
        Color color = vectorMatch(matches, 1);
        double f = 0.;
        if (matches[4].matched) {
            f = std::stod(matches[4]);
        }
        std::clog << "found: Metal " << color << " fuzz " << f << std::endl;
        return make_shared<Metal>(color, f);
    } else
    if (std::regex_match(line, matches, dielectric)) {
        double r = std::stod(matches[1]);
        std::clog << "found: Dielectric " << r << std::endl;
        return make_shared<Dielectric>(r);
    } else
    if (std::regex_match(line, matches, isotropic)) {
        Color color = vectorMatch(matches, 1);
        std::clog << "found: Isotropic " << color << std::endl;
        return make_shared<Isotropic>(color);
    } else
    if (std::regex_match(line, matches, composite)) {
        std::string name1 = matches[1];
        std::string name2 = matches[2];
        double ratio = std::stod(matches[3]);
        std::clog << "found: Composite " << name1 << ", " << name2 << ", " << ratio << std::endl;
        return make_shared<Composite>(materials[name1], materials[name2], ratio);
    }

    return {};
}

shared_ptr<Hittable> YaptSceneLoader::loadHittable(std::ifstream &file) {
    std::string line;
    std::getline(file, line);
    std::smatch matches;
    std::regex quad(R"(quad\s*=\s*(-?[0-9]*\.?[0-9]+)\s*,\s*(-?[0-9]*\.?[0-9]+)\s*,\s*(-?[0-9]*\.?[0-9]+)\s*-\s*(-?[0-9]*\.?[0-9]+)\s*,\s*(-?[0-9]*\.?[0-9]+)\s*,\s*(-?[0-9]*\.?[0-9]+)\s*-\s*(-?[0-9]*\.?[0-9]+)\s*,\s*(-?[0-9]*\.?[0-9]+)\s*,\s*(-?[0-9]*\.?[0-9]+)\s*-\s*([\w]+))");
    std::regex sphere(R"(sphere\s*=\s*(-?[0-9]+\.?[0-9]*)\s*,\s*(-?[0-9]+\.?[0-9]*)\s*,\s*(-?[0-9]+\.?[0-9]*)\s*-\s*(-?[0-9]+\.?[0-9]*)\s*-\s*([\w]+))");
    std::regex boxexp(R"(box\s*=\s*(-?[0-9]*\.?[0-9]+)\s*,\s*(-?[0-9]*\.?[0-9]+)\s*,\s*(-?[0-9]*\.?[0-9]+)\s*-\s*(-?[0-9]*\.?[0-9]+)\s*,\s*(-?[0-9]*\.?[0-9]+)\s*,\s*(-?[0-9]*\.?[0-9]+)\s*-\s*([\w]+))");
    std::regex rotate(R"(rotate\s*=\s*([xyz])\s*,\s*(-?[0-9]*\.?[0-9]+)\s*-\s*([\w]+))");
    std::regex translateexp(R"(translate\s*=\s*(-?[0-9]*\.?[0-9]+)\s*,\s*(-?[0-9]*\.?[0-9]+)\s*,\s*(-?[0-9]*\.?[0-9]+)\s*-\s*([\w]+))");
    if (std::regex_match(line, matches, quad)) {
        Vec3 origin = vectorMatch(matches, 1);
        Vec3 u = vectorMatch(matches, 4);
        Vec3 v = vectorMatch(matches, 7);
        std::string mat = matches[10];
        std::clog << "found: Quad " << origin << " ; " << u << " ; " << v << " - mat = " << mat << std::endl;
        return make_shared<Quad>(origin, u, v, materials[mat]);
    } else
    if (std::regex_match(line, matches, sphere)) {
        Vec3 center = vectorMatch(matches, 1);
        double radius = std::stod(matches[4]);
        std::string mat = matches[5];
        std::clog << "found: Sphere" << center << " ; " << radius << " - mat = " << mat << std::endl;
        return make_shared<Sphere>(center, radius, materials[mat]);
    } else
    if (std::regex_match(line, matches, boxexp)) {
        Vec3 a = vectorMatch(matches, 1);
        Vec3 b = vectorMatch(matches, 4);
        std::string mat = matches[7];
        std::clog << "found: Box " << a << " ; " << b << " - mat = " << mat << std::endl;
        return box(a, b, materials[mat]);
    } else
    if (std::regex_match(line, matches, rotate)) {
        std::string axis = matches[1];
        double angle = std::stod(matches[2]);
        std::string obj = matches[3];
        std::clog << "found: Rotate " << axis << " ; " << angle << " - obj = " << obj << std::endl;
        if (axis != "y") std::clog << "WARNING: " << axis << "-axis not supported yet" << std::endl;
        return make_shared<RotateY>(hittables[obj], angle);
    } else
    if (std::regex_match(line, matches, translateexp)) {
        Vec3 v = vectorMatch(matches, 1);
        std::string obj = matches[4];
        return make_shared<Translate>(hittables[obj], v);
    }
    return {};
}

shared_ptr<Hittable> YaptSceneLoader::loadScene(std::ifstream &file) {
    std::string line;
    std::getline(file, line);
    std::regex pattern(R"([\w]+)");
    HittableList scene;

    std::sregex_iterator iter(line.begin(), line.end(), pattern);
    std::sregex_iterator end;
    while (iter != end) {
        std::string match = iter->str();
        std::clog << "added to scene: " << match << std::endl;
        scene.add(hittables[match]);
        ++iter;
    }

    std::clog << "done parsing list" << std::endl;
    return make_shared<BVHNode>(scene);
}

shared_ptr<Hittable> YaptSceneLoader::loadLights(std::ifstream &file, shared_ptr<HittableList> lights) {
    std::string line;
    std::getline(file, line);
    std::regex pattern(R"([\w]+)");
    HittableList scene;

    std::sregex_iterator iter(line.begin(), line.end(), pattern);
    std::sregex_iterator end;
    while (iter != end) {
        std::string match = iter->str();
        std::clog << "added to lights: " << match << std::endl;
        lights->add(hittables[match]);
        ++iter;
    }

    std::clog << "done parsing list" << std::endl;

    return lights;
}

