//
// Created by franck on 24/06/24.
//

#ifndef YAPT_IMPORTER_H
#define YAPT_IMPORTER_H

#include <assimp/types.h>
#include "color.h"
#include "hittable_list.h"
#include "material.h"
#include "triangle.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

Color colorFromAi(aiColor3D aiCol) {
    return {aiCol.r, aiCol.g, aiCol.b};
}

HittableList import(std::string &path) {
    Assimp::Importer importer;
    auto scene = importer.ReadFile(path, aiProcess_Triangulate);
    std::cout << scene->mNumMeshes << " meshes" << std::endl;
    std::cout << "Root node is named: " << scene->mRootNode->mName.data << std::endl;
    auto face = scene->mMeshes[0]->mFaces[0];
    std::cout << "Face #0 has " << face.mNumIndices << " vertices" << std::endl;
    std::cout << "Face #0 has " << std::endl;
    for (auto i = 0 ; i < face.mNumIndices ; i++) {
        std::cout << " - " << face.mIndices[i] << std::endl;
    }
    auto mNumChildren = scene->mRootNode->mNumChildren;

    for (auto i = 0 ; i < mNumChildren ; i++) {
        auto node = scene->mRootNode->mChildren[i];
        auto trans = scene->mRootNode->mChildren[i]->mTransformation;
        std::cout << "child #" << i << std::endl;
        for (auto j = 0; j < 4 ; j++) {
            for (auto k = 0 ; k < 4 ; k++) {
                std::cout << trans[j][k] << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl << std::endl;
    }

    auto green = make_shared<Lambertian>(Color(.12, .45, .15));

    HittableList world;

    // number of meshes in the scene
    auto nMeshes = scene->mNumMeshes;

    for (auto i = 0 ; i < nMeshes ; i++) {
        auto mesh = scene->mMeshes[i];
        auto nVertices = mesh->mNumVertices;
        std::vector<Point3> points(nVertices);
        std::cout << "nVertices =  " << nVertices << std::endl;
        for (auto j = 0 ; j < nVertices ; j++) {
            auto vertex = mesh->mVertices[j];
            points[j] = Vec3(vertex.x, vertex.y, vertex.z);
        }
        auto nTriangles = mesh->mNumFaces;
        for (auto j = 0 ;j < nTriangles ; j++) {
            auto triangle = mesh->mFaces[j];
            std::cout << "Triangle " << j << " " << triangle.mIndices[0] << " " << triangle.mIndices[1] << " " << triangle.mIndices[2] << std::endl;
            auto A = points[triangle.mIndices[0]];
            auto B = points[triangle.mIndices[1]];
            auto C = points[triangle.mIndices[2]];

            world.add(make_shared<Triangle>(A, B-A, C-A, green));
        }
        std::cout << "world size: " << world.objects.size() << std::endl;
    }

    auto nMaterials = scene->mNumMaterials;

    std::cout << "Materials found:" << nMaterials << std::endl;

    for (auto i = 0 ; i < nMaterials ; i++ ) {
        auto material = scene->mMaterials[i];
        aiColor3D aiCol(0, 0, 0);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, aiCol);
        Color diffuseColor = colorFromAi(aiCol);

        material->Get(AI_MATKEY_COLOR_SPECULAR, aiCol);
        Color specularColor = colorFromAi(aiCol);

        material->Get(AI_MATKEY_COLOR_EMISSIVE, aiCol);
        Color emissiveColor = colorFromAi(aiCol);

        material->Get(AI_MATKEY_COLOR_AMBIENT, aiCol);
        Color ambientColor = colorFromAi(aiCol);

        material->Get(AI_MATKEY_SHININESS, aiCol);
        Color shininessColor = colorFromAi(aiCol);

        material->Get(AI_MATKEY_SHININESS_STRENGTH, aiCol);
        Color shininessStrength = colorFromAi(aiCol);

        material->Get(AI_MATKEY_REFRACTI, aiCol);
        Color refraction = colorFromAi(aiCol);

        std::cout << "Material " << i << std::endl;
        std::cout << " - Name:              " << material->GetName().C_Str() << std::endl;
        std::cout << " - Ambient:           " << ambientColor << std::endl;
        std::cout << " - Diffuse:           " << diffuseColor << std::endl;
        std::cout << " - Specular:          " << specularColor << std::endl;
        std::cout << " - Emissive:          " << emissiveColor << std::endl;
        std::cout << " - Shininess:         " << shininessColor << std::endl;
        std::cout << " - ShininessStrength: " << shininessStrength << std::endl;
        std::cout << " - RefractionIndex:   " << shininessStrength << std::endl;
    }

//    auto numProperties = material->mNumProperties;
//
//    std:: cout << "Properties found: " << numProperties << std::endl;
//    std:: cout << "Properties: " << std::endl;
//    for (int i = 0 ; i < numProperties ; i++) {
//        std::cout << " - " << material->mProperties[i] << std::endl;
//    }s

    return world;
}

#endif //YAPT_IMPORTER_H
