//
// Created by franck on 22/06/24.
//

#ifndef YAPT_DEMO_H
#define YAPT_DEMO_H

#include "yapt.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"
#include "triangle.h"
#include "quad.h"
#include "image_exporter.h"
#include "camera.h"
#include "bvh.h"

void simple_light(std::string path);
void final (std::string path);
void triangle(std::string path);
void single_triangle(std::string path);
void original(std::string path);

#endif //YAPT_DEMO_H
