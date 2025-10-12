//
// Created by franck on 21/01/25.
//


#include "parser.h"

int main(int argc, char *argv[]) {
    Parser parser;
    Scene scene;

    // load the scene description and camera
    if (!parser.parseScene(argc, argv, scene)) return 0;


    scene.camera->imageWidth = 1;
    scene.camera->initialize();

    scene.camera->render_pixel(*scene.content, *scene.lights, 0, 0);
    auto data = scene.camera->data();
    auto v = data->data[0];

    std::cout << std::endl << v << std::endl;
}
