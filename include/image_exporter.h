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

    void setRenderTime(const std::size_t renderTime) {
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
    explicit PNGImageExporter(const shared_ptr<ImageData> imageData): imageData(imageData) {}

    virtual void write(std::string fileName) override;

protected:
    shared_ptr<ImageData> imageData;
};

class EXRImageExporter final : public ImageExporter {
public:
    explicit EXRImageExporter(const shared_ptr<ImageData> imageData): imageData(imageData) {}
    void write(std::string fileName) override;

protected:
    shared_ptr<ImageData> imageData;
    size_t render_time = 0;
};

#endif //YAPT_IMAGE_EXPORTER_H
