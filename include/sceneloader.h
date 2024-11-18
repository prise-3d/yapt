//
// Created by franck on 26/09/24.
//

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
    virtual shared_ptr<Material> loadMaterial(std::ifstream& file) = 0;
    virtual shared_ptr<Hittable> loadHittable(std::ifstream &file) = 0;
    virtual shared_ptr<Hittable> loadScene(std::ifstream &file) = 0;
    virtual shared_ptr<Hittable> loadLights(std::ifstream &file, shared_ptr<HittableList> lights) = 0;
};

class YaptSceneLoader: public SceneLoader {
public:
    void load(std::string path, shared_ptr<HittableList> scene, shared_ptr<HittableList> lights, shared_ptr<Camera> camera) override;

private:
    shared_ptr<Material> loadMaterial(std::ifstream &file) override;
    shared_ptr<Hittable> loadHittable(std::ifstream &file) override;
    shared_ptr<Hittable> loadScene(std::ifstream &file) override;
    shared_ptr<Hittable> loadLights(std::ifstream &file, shared_ptr<HittableList> lights) override;

    std::unordered_map<std::string, shared_ptr<Material>> materials;
    std::unordered_map<std::string, shared_ptr<Hittable>> hittables;
};

#endif //YAPT_SCENELOADER_H
