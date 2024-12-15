//
// Created by franck on 07/06/24.
//

#include "parser.h"

int main(const int argc, char* argv[]) {
    Parser parser;
    Scene scene;

    // load the scene description and camera
    parser.parse_scene(argc, argv, scene);

    parser.start_timer();
    scene.camera->render(*scene.content, *scene.lights);
    parser.stop_timer();

    parser.export_image(argc, argv, scene);
}