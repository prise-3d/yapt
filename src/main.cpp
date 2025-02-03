//
// Created by franck on 07/06/24.
//

#include "parser.h"

int main(const int argc, char* argv[]) {
    Parser parser;
    Scene scene;

    // load the scene description and camera
    if (!parser.parseScene(argc, argv, scene)) return 0;

    parser.startTimer();
    scene.camera->render(*scene.content, *scene.lights);
    parser.stopTimer();

    parser.exportImage(argc, argv, scene);
}