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

#ifndef YAPT_SCENELOADER_H
#define YAPT_SCENELOADER_H

#include "yapt.h"
#include "camera.h"
#include "hittable_list.h"
#include <regex>
#include <fstream>

class SceneLoader {
public:
    virtual ~SceneLoader() = default;

    virtual void load(std::string path, shared_ptr <HittableList> scene, shared_ptr <HittableList> lights, shared_ptr <Camera> camera) = 0;
private:
    virtual shared_ptr<Material> load_material(std::ifstream& file) = 0;
    virtual shared_ptr<Hittable> load_hittable(std::ifstream &file) = 0;
    virtual shared_ptr<Hittable> load_scene(std::ifstream &file) = 0;
    virtual shared_ptr<Hittable> load_lights(std::ifstream &file, shared_ptr<HittableList> lights) = 0;
};

class YaptSceneLoader: public SceneLoader {
public:
    void load(std::string path, shared_ptr<HittableList> scene, shared_ptr<HittableList> lights, shared_ptr<Camera> camera) override;

private:
    shared_ptr<Material> load_material(std::ifstream &file) override;
    shared_ptr<Hittable> load_hittable(std::ifstream &file) override;
    shared_ptr<Hittable> load_scene(std::ifstream &file) override;
    shared_ptr<Hittable> load_lights(std::ifstream &file, shared_ptr<HittableList> lights) override;

    std::unordered_map<std::string, shared_ptr<Material>> materials;
    std::unordered_map<std::string, shared_ptr<Hittable>> hittables;
};

#endif //YAPT_SCENELOADER_H
