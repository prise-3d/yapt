/*
* This file is part of the YAPT distribution (https://github.com/prise-3d/yapt).
 * Copyright (c) 2025 PrISE-3D.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * --- ADDITIONAL PERMISSION UNDER GNU GPL VERSION 3 SECTION 7 ---
 *
 * If you modify this Program, or any covered work, by linking or
 * combining it with the Intel Math Kernel Library (MKL) (or a modified
 * version of that library), containing parts covered by the terms of
 * the Intel Simplified Software License, the licensors of this
 * Program grant you additional permission to convey the resulting work.
 */

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
            materials[matches[1]] = load_material(file);
        } else
        if (std::regex_match(line, matches, object)) {
            std::clog << "found: Object " << matches[1] << std::endl;
            hittables[matches[1]] = load_hittable(file);
        } else
        if (std::regex_match(line, matches, sceneexp)) {
            std::clog << "found: Scene " << matches[1] << std::endl;
            std::clog << "hittables found so far..." << std::endl;
            for (const auto& pair : hittables) {
                std::clog << " - key: " << pair.first << ", value: " << pair.second << std::endl;
            }
               std::clog << "materials found so far..." << std::endl;
            for (const auto& pair : materials) {
                std::clog << " - key: " << pair.first << ", value: " << pair.second << std::endl;
            }

            scene->add(load_scene(file));
        } else
        if (std::regex_match(line, matches, lightsexp)) {
            std::clog << "found: Lights " << matches[1] << std::endl;
            scene->add(load_lights(file, lights));
        }
    }
}

shared_ptr<Material> YaptSceneLoader::load_material(std::ifstream &file) {
    std::string line;
    std::getline(file, line);
    std::regex lambertian(R"(lambertian\s*=\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+))");
    std::regex lambertian_checker(R"(lambertian_checker\s*=\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+))");
    std::regex difflight(R"(difflight\s*=\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+))");
    std::regex metal(R"(metal\s*=\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+)(?:\s*,\s*([0-9]*\.?[0-9]+))?)");
    std::regex dielectric(R"(dielectric\s*=\s*([0-9]*\.?[0-9]+))");
    std::regex isotropic(R"(isotropic\s*=\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+)\s*,\s*([0-9]*\.?[0-9]+))");
    std::smatch matches;

    if (std::regex_match(line, matches, lambertian)) {
        Color color = vectorMatch(matches, 1);
        std::clog << "found: Lambertian " << color << std::endl;
        return make_shared<Lambertian>(color);
        //return make_shared<Lambertian>(make_shared<CheckerTexture>(15, Color(0, 0, 0), color));
    } else if (std::regex_match(line, matches, lambertian_checker)) {
        std::cout << "MATCHING" << line << std::endl;
        double scale = std::stod(matches[1]);
        Color color1 = vectorMatch(matches, 2);
        Color color2 = vectorMatch(matches, 5);
        return make_shared<Lambertian>(make_shared<CheckerTexture>(scale, color1, color2));
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
    }

    return {};
}

shared_ptr<Hittable> YaptSceneLoader::load_hittable(std::ifstream &file) {
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

shared_ptr<Hittable> YaptSceneLoader::load_scene(std::ifstream &file) {
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

shared_ptr<Hittable> YaptSceneLoader::load_lights(std::ifstream &file, shared_ptr<HittableList> lights) {
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


