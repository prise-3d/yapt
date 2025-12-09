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

#ifndef YAPT_IMAGE_EXPORTER_H
#define YAPT_IMAGE_EXPORTER_H

#include "yapt.h"
#include <string>
#include "image_data.h"
#include <memory>

#include "camera.h"

class ImageExporter {
public:
    ImageExporter(): renderTime(0) {}

    void set_render_time(const std::size_t renderTime) {
        this->renderTime = renderTime;
    }

    std::size_t getRenderTime() const{
        return renderTime;
    }

    virtual ~ImageExporter() = default;
    virtual void write(std::string path) = 0;

protected:
    std::size_t renderTime;
};

class PNGImageExporter final : public ImageExporter {
public:
    explicit PNGImageExporter(const shared_ptr<ImageData> image_data): image_data(image_data) {}

    virtual void write(std::string fileName) override;

protected:
    shared_ptr<ImageData> image_data;
};

class EXRImageExporter final : public ImageExporter {
public:
    explicit EXRImageExporter(const shared_ptr<ImageData> image_data): image_data(image_data) {}
    void write(std::string fileName) override;

protected:
    shared_ptr<ImageData> image_data;
    size_t render_time = 0;
};

#endif //YAPT_IMAGE_EXPORTER_H
