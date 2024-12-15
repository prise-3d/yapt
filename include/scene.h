//
// Created by franck on 15/12/24.
//

#ifndef SCENE_H
#define SCENE_H

#include "yapt.h"
#include "hittable_list.h"
#include "camera.h"

class Scene {
public:
    Scene() {}
    Scene(std::shared_ptr<HittableList> content, std::shared_ptr<HittableList> lights, std::shared_ptr<Camera> camera): content(content), lights(lights), camera(camera) {}
    ~Scene() = default;
    std::shared_ptr<HittableList> content;
    std::shared_ptr<HittableList> lights;
    std::shared_ptr<Camera> camera;
};

#endif //SCENE_H
