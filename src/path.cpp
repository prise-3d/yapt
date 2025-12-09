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

#include "path.h"

Path::Path(const std::size_t max_depth) : steps(std::vector<PathStep>(max_depth)) {}

Path::Path(std::size_t max_depth, PathStep &step) : steps(std::vector<PathStep>(max_depth)) {
    append(step);
}

void Path::append(const PathStep &step) {
    steps[depth++] = step;
}

PathStep Path::get(const size_t index) const {
    return steps[index];
}

std::size_t Path::getDepth() const {
    return depth;
}

size_t Path::getMaxDepth() const {
    return steps.size();
}

void Path::registerScatterRecord(const ScatterRecord &scatterRecord) {
    steps[depth - 1].registerScatterRecord(scatterRecord);
}


bool PathGuidingStrategy::visible(const Vec3 &p, const Vec3 &q) const {
    HitRecord record;
    const Ray r(p, q-p);
    return scene->hit(r, Interval(0.0001, 1), record);
}

bool PathGuidingStrategy::connect(Path &cameraPath, const Path &lightPath) const {
    PathStep cameraStep = cameraPath.lastStep();
    PathStep lightStep = lightPath.lastStep();

    const Point3 endPoint = cameraStep.hitRecord.p;
    const Point3 startPoint = lightStep.hitRecord.p;

    const Ray ray(endPoint, startPoint - endPoint);

    if (HitRecord record; scene->hit(ray, Interval(0.0001, 1 - .0001), record)) {
        return false;
    }

    size_t size = lightPath.getDepth();
    for (size_t i = 0 ; i < size ; i++) {
        size_t index = size - i - 1;
        PathStep step = lightPath.get(index);
        cameraPath.append(step);
    }

    return true;
}

Ray Path::incomingRay() const {
    const auto last = steps[depth - 1];
    const auto previous = steps[depth - 2];

    Point3 origin = previous.hitRecord.p;
    const Point3 point = last.hitRecord.p;

    return {origin, point - origin};
}

PathStep Path::lastStep() const {
    return steps[depth - 1];
}

/**
 * Will try to grow a path. If grown successfully:
 *   - a ScatterRecord describing the outgoing interaction with the scene will be registered to the last known PathStep
 *   - a new PathStep will be added, containing the HitRecord describing the incoming interaction with the scene.
 * Growing a path may fail for two reasons:
 *   - the path is grown in a scattering direction to the infinite
 *   - the material at the path position does not scatter light
 * @param path the path to grow.
 * @return true if the path was successfully grown
 */
bool SimpleGuidingStrategy::grow(Path &path) {
    // TODO: we still need to account for light contributions!

    const auto lastStep = path.lastStep();
    const auto incomingRay = path.incomingRay();

    const HitRecord hitRecord = lastStep.hitRecord;

    Color color_from_emission = hitRecord.mat->emitted(incomingRay, hitRecord, hitRecord.u, hitRecord.v, hitRecord.p);

    ScatterRecord scatterRecord;

    // does the material scatter an outgoing ray?
    // captures a ScatterRecord to describe what happens at the surface of the material
    const bool isScattering = hitRecord.mat->scatter(incomingRay, hitRecord, scatterRecord);

    // registers the scatter record as the know scattering behaviour for the last step
    path.registerScatterRecord(scatterRecord);

    if (!isScattering) {
        // the material does not scatter incoming light (eg. it is purely emissive)
        // return colorFromEmission; // what should we do?
        //TODO: is this correct?
        return false;
    }

    // the material scatters a ray
    Ray scatteredRay;
    HitRecord nextRecord;

    if (scatterRecord.skip_pdf) {
        // the material is purely specular
        scatteredRay = scatterRecord.skip_pdf_ray;
    } else {
        // the material scatters an outgoing ray with a non dirac bsdf
        scatteredRay = {hitRecord.p, scatterRecord.pdf_ptr->generate()};
    }

    if (!scene->hit(scatteredRay, Interval(0.001, infinity), nextRecord)) {
        // what shall we do?
        return false;
    }

    path.append(PathStep(nextRecord));
    // all is well that ends well
    return true;
}










