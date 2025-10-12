//
// Created by franck on 17/06/24.
//

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
